#include "mime.h"
#include "rbk/string/comparator.h"
#include <map>
using namespace std;

using Mappa                    = std::map<std::string, std::string, StringCompare>;
static const Mappa mimeTypeMap = {
    {"htm", "text/html"},
    {"html", "text/html"},
    {"php", "text/html"},
    {"css", "text/css"},
    {"txt", "text/plain"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"xml", "application/xml"},
    {"swf", "application/x-shockwave-flash"},
    {"flv", "video/x-flv"},
    {"png", "image/png"},
    {"jpe", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"gif", "image/gif"},
    {"bmp", "image/bmp"},
    {"ico", "image/vnd.microsoft.icon"},
    {"tiff", "image/tiff"},
    {"tif", "image/tiff"},
    {"svg", "image/svg+xml"},
    {"svgz", "image/svg+xml"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"eot", "font/eot"},
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"mp4", "video/mp4"},
    {"webm", "video/webm"},
    {"ogg", "video/ogg"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"zip", "application/zip"},
    {"gz", "application/gzip"},
    {"tar", "application/tar"},
    {"rar", "application/rar"},
    {"7z", "application/x-7z-compressed"},
    {"csv", "text/csv"},
    {"pdf", "application/pdf"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls", "application/vnd.ms-excel"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"odt", "application/vnd.oasis.opendocument.text"},
    {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {"odp", "application/vnd.oasis.opendocument.presentation"},
    {"odg", "application/vnd.oasis.opendocument.graphics"},
    {"odc", "application/vnd.oasis.opendocument.chart"},
    {"odb", "application/vnd.oasis.opendocument.database"},
    {"odf", "application/vnd.oasis.opendocument.formula"},
    {"odm", "application/vnd.oasis.opendocument.text-master"},
    {"ott", "application/vnd.oasis.opendocument.text-template"},
    {"ots", "application/vnd.oasis.opendocument.spreadsheet-template"},
    {"otp", "application/vnd.oasis.opendocument.presentation-template"},
    {"otg", "application/vnd.oasis.opendocument.graphics-template"},
    {"otc", "application/vnd.oasis.opendocument.chart-template"},
    {"otb", "application/vnd.oasis.opendocument.database-template"},
    {"otf", "application/vnd.oasis.opendocument.formula-template"},
    {"otm", "application/vnd.oasis.opendocument.text-master"},
    {"oth", "application/vnd.oasis.opendocument.text-web"},
    {"rtf", "application/rtf"},
    {"wasm", "application/wasm"},
    {"xhtml", "application/xhtml+xml"},
    {"xul", "application/vnd.mozilla.xul+xml"},
    {"webp", "image/webp"},
    {"appcache", "text/cache-manifest"},
    {"manifest", "text/cache-manifest"},
    {"htc", "text/x-component"},
    {"vcf", "text/x-vcard"},
    {"vcard", "text/x-vcard"},
    {"vtt", "text/vtt"},
    {"m3u8", "application/x-mpegURL"},
    {"m3u", "audio/x-mpegurl"},
    {"ics", "text/calendar"},
    {"ical", "text/calendar"},
    {"coffee", "text/coffeescript"},
    {"less", "text/css"},
    {"scss", "text/css"},
    {"sass", "text/css"},
    {"mjs", "application/javascript"},
    {"map", "application/json"},
    {"lock", "application/json"},
    {"webmanifest", "application/manifest+json"},
    {"appcache", "text/cache-manifest"},
    {"manifest", "text/cache-manifest"},
    {"htc", "text/x-component"},
    {"vcf", "text/x-vcard"},
    {"vcard", "text/x-vcard"},
    {"vtt", "text/vtt"},
    {"m3u8", "application/x-mpegURL"},
    {"m3u", "audio/x-mpegurl"},
};

const Mappa* getMimeTypeMap() {
	return &mimeTypeMap;
}

std::string_view getMimeType(std::string_view file) {
	auto const pos = file.rfind('.');
	if (pos == string_view::npos) {
		return string_view{};
	}
	auto ext = file.substr(pos + 1);
	if (auto iter = mimeTypeMap.find(ext); iter != mimeTypeMap.end()) {
		return iter->second;
	}
	return "text/plaintext";
}
