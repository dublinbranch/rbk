#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/json.hpp>
#include <boost/system/error_code.hpp>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace bj = boost::json;
namespace fs = std::filesystem;
using tcp = boost::asio::ip::tcp;

namespace {

std::mutex g_log_mu;

std::mutex g_restic_job_mx;
bool g_restic_job_busy = false;

bool try_begin_restic_job() {
  std::lock_guard<std::mutex> lk(g_restic_job_mx);
  if (g_restic_job_busy) {
    return false;
  }
  g_restic_job_busy = true;
  return true;
}

void end_restic_job() {
  std::lock_guard<std::mutex> lk(g_restic_job_mx);
  g_restic_job_busy = false;
}

void log_msg(std::string_view s) {
  std::lock_guard<std::mutex> lk(g_log_mu);
  std::cerr << "[beast] " << s << '\n';
}

enum class Action { kNone, kSnapshots, kBackup };

std::string_view target_path(std::string_view t) {
  auto q = t.find('?');
  if (q != std::string_view::npos) {
    return t.substr(0, q);
  }
  return t;
}

std::string pack_status(std::string_view msg) {
  bj::object o;
  o.emplace("type", "status");
  o.emplace("message", msg);
  return bj::serialize(o);
}

std::string pack_line(std::string_view line) {
  bj::object o;
  o.emplace("type", "restic_line");
  o.emplace("line", line);
  return bj::serialize(o);
}

std::string pack_exit(int code) {
  bj::object o;
  o.emplace("type", "exit");
  o.emplace("code", code);
  return bj::serialize(o);
}

std::string pack_error(std::string_view msg) {
  bj::object o;
  o.emplace("type", "error");
  o.emplace("message", msg);
  return bj::serialize(o);
}

Action parse_action(std::string_view body) {
  try {
    bj::value jv = bj::parse(body);
    if (!jv.is_object()) {
      return Action::kNone;
    }
    bj::object& obj = jv.as_object();
    auto const it = obj.find("action");
    if (it == obj.end() || !it->value().is_string()) {
      return Action::kNone;
    }
    std::string a(it->value().as_string());
    for (auto& ch : a) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (a == "snapshots") {
      return Action::kSnapshots;
    }
    if (a == "backup") {
      return Action::kBackup;
    }
  } catch (...) {
    return Action::kNone;
  }
  return Action::kNone;
}

/** POST /api/restic body: {"op":"snapshots"|"backup"} — same semantics as beast2. */
Action parse_http_op(std::string_view body) {
  try {
    bj::value jv = bj::parse(body);
    if (!jv.is_object()) {
      return Action::kNone;
    }
    bj::object& obj = jv.as_object();
    auto const it = obj.find("op");
    if (it == obj.end() || !it->value().is_string()) {
      return Action::kNone;
    }
    std::string op(it->value().as_string());
    for (auto& ch : op) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (op == "snapshots") {
      return Action::kSnapshots;
    }
    if (op == "backup") {
      return Action::kBackup;
    }
  } catch (...) {
    return Action::kNone;
  }
  return Action::kNone;
}

fs::path index_html_path() {
  if (char const* env = std::getenv("BEAST_WEB_ROOT")) {
    return fs::path(env) / "index.html";
  }
  return fs::path("web") / "index.html";
}

fs::path restic_gui_html_path() {
  if (char const* env = std::getenv("BEAST_WEB_ROOT")) {
    return fs::path(env) / "restic.html";
  }
  return fs::path("web") / "restic.html";
}

bool read_utf8_file(fs::path const& p, std::string& out) {
  std::ifstream f(p, std::ios::binary);
  if (!f) {
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  return true;
}

void http_error(tcp::socket& sock, http::status status, unsigned version,
                std::string_view body) {
  http::response<http::string_body> res{status, version};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/plain; charset=utf-8");
  res.body() = std::string(body);
  res.prepare_payload();
  http::write(sock, res);
}

std::string home_dir() {
  char const* h = std::getenv("HOME");
  return h ? std::string(h) : std::string("/");
}

std::string backup_cwd() {
  char const* d = std::getenv("RESTIC_BACKUP_CWD");
  return d ? std::string(d) : home_dir();
}

std::string repo_path() {
  return home_dir() + "/deletein/restic/repo";
}

char const* action_name(Action a) {
  return a == Action::kSnapshots ? "snapshots" : "backup";
}

struct ResticSpawn {
  int out_fd = -1;
  pid_t pid = -1;
  int err = 0;
};

ResticSpawn spawn_restic(Action action) {
  ResticSpawn r;
  int link[2];
  if (::pipe(link) != 0) {
    r.err = errno;
    log_msg(std::string("spawn_restic: pipe failed errno=") + std::to_string(errno));
    return r;
  }

  pid_t const pid = ::fork();
  if (pid < 0) {
    r.err = errno;
    ::close(link[0]);
    ::close(link[1]);
    log_msg(std::string("spawn_restic: fork failed errno=") + std::to_string(errno));
    return r;
  }

  if (pid == 0) {
    ::close(link[0]);
    if (::dup2(link[1], STDOUT_FILENO) < 0) {
      std::_Exit(126);
    }
    if (::dup2(link[1], STDERR_FILENO) < 0) {
      std::_Exit(126);
    }
    ::close(link[1]);

    std::string const cwd = backup_cwd();
    if (::chdir(cwd.c_str()) != 0) {
      std::_Exit(126);
    }

    if (::setenv("RESTIC_PASSWORD", "happy", 1) != 0) {
      std::_Exit(126);
    }

    std::string const repo = repo_path();
    std::vector<std::string> store;
    store.reserve(8);
    store.emplace_back("restic");
    store.emplace_back("-r");
    store.push_back(repo);
    if (action == Action::kSnapshots) {
      store.emplace_back("snapshots");
      store.emplace_back("--json");
    } else {
      store.emplace_back("backup");
      store.emplace_back("Public/boost/");
      store.emplace_back("--json");
    }

    std::vector<char*> argv;
    argv.reserve(store.size() + 1);
    for (auto& s : store) {
      argv.push_back(s.data());
    }
    argv.push_back(nullptr);

    ::execvp(argv[0], argv.data());
    std::_Exit(127);
  }

  ::close(link[1]);
  int flags = ::fcntl(link[0], F_GETFL, 0);
  if (flags >= 0) {
    ::fcntl(link[0], F_SETFL, flags | O_NONBLOCK);
  }

  r.out_fd = link[0];
  r.pid = pid;
  log_msg(std::string("restic child pid=") + std::to_string(pid) + " action=" +
          action_name(action) + " cwd=" + backup_cwd() + " repo=" + repo_path());
  return r;
}

bool ws_send_text(websocket::stream<tcp::socket>& ws, std::string const& payload) {
  beast::error_code ec;
  ws.text(true);
  ws.write(net::buffer(payload), ec);
  if (ec) {
    log_msg(std::string("ws write failed len=") + std::to_string(payload.size()) +
            " ec=" + ec.message());
    return false;
  }
  return true;
}

void tcp_write_raw(tcp::socket& sock, std::string_view data,
                   boost::system::error_code& ec) {
  net::write(sock, net::buffer(data.data(), data.size()), ec);
}

void http_write_chunk(tcp::socket& sock, std::string_view payload,
                      boost::system::error_code& ec) {
  char lenhex[40];
  int const nw =
      std::snprintf(lenhex, sizeof(lenhex), "%zx\r\n", payload.size());
  if (nw <= 0) {
    ec.assign(EIO, boost::system::generic_category());
    return;
  }
  tcp_write_raw(sock, std::string_view(lenhex, static_cast<std::size_t>(nw)), ec);
  if (ec) {
    return;
  }
  tcp_write_raw(sock, payload, ec);
  if (ec) {
    return;
  }
  tcp_write_raw(sock, "\r\n", ec);
}

void send_chunked_ndjson_headers(tcp::socket& sock, boost::system::error_code& ec) {
  std::ostringstream oss;
  oss << "HTTP/1.1 200 OK\r\n"
      << "Server: " << BOOST_BEAST_VERSION_STRING << "\r\n"
      << "Content-Type: application/x-ndjson; charset=utf-8\r\n"
      << "Transfer-Encoding: chunked\r\n"
      << "Connection: close\r\n"
      << "\r\n";
  std::string const hdr = oss.str();
  tcp_write_raw(sock, hdr, ec);
}

/** Stream raw restic stdout as chunked NDJSON; closes pipe, waitpid; returns exit code or -1. */
int drain_restic_pipe_chunked(tcp::socket& sock, int restic_out, pid_t restic_pid) {
  std::string partial_restic;
  char raw[8192];

  for (;;) {
    struct pollfd fds {};
    fds.fd = restic_out;
    fds.events = POLLIN | POLLHUP;

    int pr = ::poll(&fds, 1, -1);
    if (pr < 0) {
      if (errno == EINTR) {
        continue;
      }
      ::kill(restic_pid, SIGKILL);
      int st = 0;
      (void)::waitpid(restic_pid, &st, 0);
      ::close(restic_out);
      return -1;
    }

    if (fds.revents & (POLLERR | POLLNVAL)) {
      ::kill(restic_pid, SIGKILL);
      int st = 0;
      (void)::waitpid(restic_pid, &st, 0);
      ::close(restic_out);
      return -1;
    }

    if ((fds.revents & (POLLIN | POLLHUP)) == 0) {
      continue;
    }

    bool eof = false;
    boost::system::error_code ec;
    for (;;) {
      ssize_t const n = ::read(restic_out, raw, sizeof(raw));
      if (n < 0) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == EAGAIN) {
          break;
        }
        ::kill(restic_pid, SIGKILL);
        int st = 0;
        (void)::waitpid(restic_pid, &st, 0);
        ::close(restic_out);
        return -1;
      }
      if (n == 0) {
        eof = true;
        break;
      }

      partial_restic.append(raw, static_cast<std::size_t>(n));
      std::size_t pos = 0;
      while ((pos = partial_restic.find('\n')) != std::string::npos) {
        std::string line(partial_restic.data(), pos);
        partial_restic.erase(0, pos + 1);
        line.push_back('\n');
        http_write_chunk(sock, line, ec);
        if (ec) {
          ::kill(restic_pid, SIGKILL);
          int st = 0;
          (void)::waitpid(restic_pid, &st, 0);
          ::close(restic_out);
          return -1;
        }
      }
    }

    if (eof) {
      if (!partial_restic.empty()) {
        partial_restic.push_back('\n');
        http_write_chunk(sock, partial_restic, ec);
        if (ec) {
          ::kill(restic_pid, SIGKILL);
          int st = 0;
          (void)::waitpid(restic_pid, &st, 0);
          ::close(restic_out);
          return -1;
        }
      }
      ::close(restic_out);
      int st = 0;
      int code = -1;
      if (::waitpid(restic_pid, &st, 0) >= 0 && WIFEXITED(st)) {
        code = WEXITSTATUS(st);
      }
      return code;
    }
  }
}

void handle_api_restic(tcp::socket sock, http::request<http::string_body> req) {
  unsigned const ver = req.version();
  std::string const& body = req.body();

  constexpr std::size_t kMaxBody = 65536;
  if (body.size() > kMaxBody) {
    http_error(sock, http::status::payload_too_large, ver,
               "request body too large\r\n");
    return;
  }

  Action const act = parse_http_op(body);
  if (act == Action::kNone) {
    http_error(sock, http::status::bad_request, ver,
               "expected JSON object with \"op\": \"snapshots\" or \"backup\"\r\n");
    return;
  }

  if (!try_begin_restic_job()) {
    http_error(sock, http::status::conflict, ver,
               "restic job already running\r\n");
    return;
  }

  ResticSpawn sp = spawn_restic(act);
  if (sp.out_fd < 0) {
    end_restic_job();
    http_error(sock, http::status::internal_server_error, ver,
               std::string("failed to spawn restic: ") + std::strerror(sp.err) + "\r\n");
    return;
  }

  boost::system::error_code ec;
  send_chunked_ndjson_headers(sock, ec);
  if (ec) {
    ::kill(sp.pid, SIGKILL);
    int st = 0;
    (void)::waitpid(sp.pid, &st, 0);
    ::close(sp.out_fd);
    end_restic_job();
    log_msg(std::string("api/restic: header write failed ") + ec.message());
    return;
  }

  log_msg("POST /api/restic streaming NDJSON (chunked)");

  int const code = drain_restic_pipe_chunked(sock, sp.out_fd, sp.pid);

  std::string const tail =
      std::string(R"({"message_type":"server_exit","exit_code":)") +
      std::to_string(code) + "}\n";
  http_write_chunk(sock, tail, ec);
  if (!ec) {
    tcp_write_raw(sock, "0\r\n\r\n", ec);
  }

  end_restic_job();

  beast::error_code sec;
  sock.shutdown(tcp::socket::shutdown_send, sec);
}

void serve_http_index(tcp::socket& sock, unsigned version, bool keep_alive) {
  fs::path const path = index_html_path();
  std::string html;
  if (!read_utf8_file(path, html)) {
    log_msg(std::string("missing UI file: ") + path.string() +
            " (set BEAST_WEB_ROOT or create web/index.html)");
    http_error(sock, http::status::not_found, version,
               "UI not found. Place web/index.html or set BEAST_WEB_ROOT.\r\n");
    return;
  }

  http::response<http::string_body> res{http::status::ok, version};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/html; charset=utf-8");
  res.keep_alive(keep_alive);
  res.body() = std::move(html);
  res.prepare_payload();
  http::write(sock, res);
}

void serve_http_restic_gui(tcp::socket& sock, unsigned version, bool keep_alive) {
  fs::path const path = restic_gui_html_path();
  std::string html;
  if (!read_utf8_file(path, html)) {
    log_msg(std::string("missing Restic HTTP UI: ") + path.string() +
            " (set BEAST_WEB_ROOT or create web/restic.html)");
    http_error(sock, http::status::not_found, version,
               "Restic UI not found. Place web/restic.html or set BEAST_WEB_ROOT.\r\n");
    return;
  }

  http::response<http::string_body> res{http::status::ok, version};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/html; charset=utf-8");
  res.keep_alive(keep_alive);
  res.body() = std::move(html);
  res.prepare_payload();
  http::write(sock, res);
}

void websocket_session(tcp::socket socket,
                       http::request<http::string_body> const& upgrade_req) {
  beast::flat_buffer buf;

  try {
    websocket::stream<tcp::socket> ws{std::move(socket)};

    ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    ws.accept(upgrade_req);
    log_msg("websocket handshake complete");

    int const native = ws.next_layer().native_handle();

    if (!ws_send_text(ws, pack_status("connected"))) {
      return;
    }

    int restic_out = -1;
    pid_t restic_pid = -1;
    std::string partial_restic;

    bool active = true;
    while (active) {
      struct pollfd fds[2];
      int nfds = 1;
      fds[0].fd = native;
      fds[0].events = POLLIN;
      if (restic_out >= 0) {
        fds[1].fd = restic_out;
        fds[1].events = POLLIN;
        nfds = 2;
      }

      int pr = ::poll(fds, nfds, -1);
      if (pr < 0) {
        if (errno == EINTR) {
          continue;
        }
        break;
      }

      if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        break;
      }

      // Do not treat POLLHUP as fatal here: when restic exits, Linux often sets
      // POLLHUP on the read end once the write end is closed. We must read() until
      // EOF and then send pack_exit; handling POLLHUP here skipped that and left
      // the UI stuck with no final message.
      if (nfds == 2 && (fds[1].revents & (POLLERR | POLLNVAL))) {
        log_msg("restic pipe error");
        if (restic_out >= 0) {
          ::close(restic_out);
          restic_out = -1;
        }
        if (restic_pid > 0) {
          int st = 0;
          ::waitpid(restic_pid, &st, 0);
          restic_pid = -1;
        }
        end_restic_job();
        partial_restic.clear();
        if (!ws_send_text(ws, pack_error("restic pipe failed"))) {
          active = false;
        }
        continue;
      }

      if (nfds == 2 && (fds[1].revents & (POLLIN | POLLHUP))) {
        char raw[8192];
        bool eof = false;
        for (;;) {
          ssize_t const n = ::read(restic_out, raw, sizeof(raw));
          if (n < 0) {
            if (errno == EINTR) {
              continue;
            }
            if (errno == EAGAIN) {
              break;
            }
            log_msg(std::string("restic pipe read errno=") + std::to_string(errno));
            active = false;
            break;
          }
          if (n == 0) {
            eof = true;
            break;
          }

          partial_restic.append(raw, static_cast<std::size_t>(n));
          std::size_t pos = 0;
          while ((pos = partial_restic.find('\n')) != std::string::npos) {
            std::string_view const line(partial_restic.data(), pos);
            if (!ws_send_text(ws, pack_line(line))) {
              active = false;
              break;
            }
            partial_restic.erase(0, pos + 1);
          }
          if (!active) {
            break;
          }
        }

        if (!active) {
          break;
        }

        if (eof) {
          if (!partial_restic.empty()) {
            if (!ws_send_text(ws, pack_line(partial_restic))) {
              active = false;
              break;
            }
            partial_restic.clear();
          }
          ::close(restic_out);
          restic_out = -1;
          int st = 0;
          int code = -1;
          if (restic_pid > 0) {
            if (::waitpid(restic_pid, &st, 0) >= 0) {
              code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
            }
            restic_pid = -1;
          }
          end_restic_job();
          log_msg(std::string("restic finished exit_code=") + std::to_string(code));
          if (!ws_send_text(ws, pack_exit(code))) {
            active = false;
            break;
          }
        }
      }

      if (!active) {
        break;
      }

      if ((fds[0].revents & POLLIN) != 0) {
        beast::error_code ec;
        buf.consume(buf.size());
        ws.read(buf, ec);
        if (ec == websocket::error::closed) {
          active = false;
          break;
        }
        if (ec) {
          log_msg(std::string("ws read error: ") + ec.message());
          break;
        }

        std::string cmd = beast::buffers_to_string(buf.data());
        buf.consume(buf.size());

        Action const act = parse_action(cmd);
        if (act == Action::kNone) {
          log_msg(std::string("ws recv unknown cmd (len=") +
                  std::to_string(cmd.size()) + "): " +
                  cmd.substr(0, std::min(cmd.size(), size_t{120})));
          if (!ws_send_text(ws, pack_error("unknown or missing action"))) {
            active = false;
          }
          continue;
        }

        if (!try_begin_restic_job()) {
          log_msg("ws reject: another restic job is already running");
          if (!ws_send_text(ws, pack_error("a restic job is already running"))) {
            active = false;
          }
          continue;
        }

        log_msg(std::string("ws action=") + action_name(act));

        ResticSpawn sp = spawn_restic(act);
        if (sp.out_fd < 0) {
          end_restic_job();
          if (!ws_send_text(ws, pack_error(std::string("failed to start restic: ") +
                                           std::strerror(sp.err)))) {
            active = false;
          }
          continue;
        }

        restic_out = sp.out_fd;
        restic_pid = sp.pid;
        partial_restic.clear();

        if (act == Action::kSnapshots) {
          if (!ws_send_text(ws, pack_status("started snapshots"))) {
            active = false;
          }
        } else {
          if (!ws_send_text(ws, pack_status("started backup Public/boost/"))) {
            active = false;
          }
        }
        if (!active) {
          break;
        }
      }
    }

    if (restic_out >= 0) {
      ::close(restic_out);
      restic_out = -1;
    }
    if (restic_pid > 0) {
      ::kill(restic_pid, SIGTERM);
      int st = 0;
      ::waitpid(restic_pid, &st, 0);
      restic_pid = -1;
      end_restic_job();
    }

    beast::error_code ec;
    ws.close(websocket::close_code::normal, ec);
  } catch (std::exception const& e) {
    std::cerr << "websocket_session: " << e.what() << '\n';
  }
}

void session(tcp::socket socket) {
  try {
    try {
      auto const ep = socket.remote_endpoint();
      log_msg("connection " + ep.address().to_string() + ":" +
              std::to_string(ep.port()));
    } catch (...) {
      log_msg("connection (peer endpoint unavailable)");
    }

    beast::flat_buffer buffer;
    http::request<http::string_body> req;
    http::read(socket, buffer, req);

    std::string_view const path = target_path(req.target());
    log_msg(std::string("HTTP ") + std::string(to_string(req.method())) + " " +
            std::string(path));

    if (req.method() == http::verb::get && path == "/") {
      serve_http_index(socket, req.version(), false);
      return;
    }

    if (req.method() == http::verb::get && path == "/api/restic") {
      serve_http_restic_gui(socket, req.version(), false);
      return;
    }

    if (req.method() == http::verb::post && path == "/api/restic") {
      handle_api_restic(std::move(socket), std::move(req));
      return;
    }

    if (req.method() == http::verb::get && path == "/ws") {
      if (!websocket::is_upgrade(req)) {
        http_error(socket, http::status::bad_request, req.version(),
                   "Expected WebSocket upgrade request\r\n");
        beast::error_code ec;
        socket.shutdown(tcp::socket::shutdown_send, ec);
        return;
      }
      websocket_session(std::move(socket), req);
      return;
    }

    http_error(socket, http::status::not_found, req.version(), "Not found\r\n");
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
  } catch (std::exception const& e) {
    std::cerr << "session: " << e.what() << '\n';
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  signal(SIGPIPE, SIG_IGN);

  const char* host = "0.0.0.0";
  unsigned short port = 8080;

  if (argc >= 2) {
    port = static_cast<unsigned short>(std::atoi(argv[1]));
  }
  if (argc >= 3) {
    host = argv[2];
  }

  try {
    net::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {net::ip::make_address(host), port}};
    std::cout << "Listening on http://" << host << ':' << port << '\n';
    std::cerr << "[beast] each connection is handled in its own thread; "
                 "see stderr for activity.\n";

    for (;;) {
      tcp::socket socket{ioc};
      acceptor.accept(socket);
      std::thread([s = std::move(socket)]() mutable {
        session(std::move(s));
      }).detach();
    }
  } catch (std::exception const& e) {
    std::cerr << "fatal: " << e.what() << '\n';
    return 1;
  }
}
