#pragma once

// Minimal PPPLC Config mirror for intrusive decode tests (see PPPLC/misc/describeconfig.h).

#include "rbk/HTTP/beastConfig.h"
#include "rbk/HTTP/beastConfigDescribe.h"
#include "rbk/minMysql/DBConf.h"
#include "rbk/minMysql/DBConfDescribe.h"
#include "rbk/misc/NanoSpammerConfigDescribe.h"
#include "rbk/misc/QDebugConfig.h"

#include <boost/describe.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/json/conversion.hpp>

#include <QString>
#include <optional>
#include <string>

namespace wm_config {

struct DBMaintainance {
	bool checkCongruency = false;
	bool refreshDBSchema = false;
};
BOOST_DESCRIBE_STRUCT(DBMaintainance, (), (checkCongruency, refreshDBSchema))

struct Http : public BeastConf {};
BOOST_DESCRIBE_STRUCT(Http, (BeastConf), ())

struct MinMax {
	uint min = 0;
	uint max = 0;
};
BOOST_DESCRIBE_STRUCT(MinMax, (), (min, max))

struct Stepper {
	MinMax mm;
	MinMax step;
};
BOOST_DESCRIBE_STRUCT(Stepper, (), (mm, step))

struct MachineStepper {
	Stepper corta;
	Stepper lunga;
};
BOOST_DESCRIBE_STRUCT(MachineStepper, (), (corta, lunga))

struct MachineParameter {
	Stepper        carriagePosition;
	MachineStepper carriagePositions;
	Stepper        boardPosition;
	Stepper        jetPosition;
	std::string    machineSerialCode;
	uint           pressureIntensityLevels = 2;
};
BOOST_DESCRIBE_STRUCT(MachineParameter, (),
                      (boardPosition, carriagePositions, jetPosition, machineSerialCode, pressureIntensityLevels))

struct Devel {
	bool                cacheBusterOn = true;
	uint                sqlCache       = 0;
	bool                simulateMachine = false;
	bool                setMachineCheck = false;
	std::optional<bool> redirectIfPythonFail = true;
	std::optional<bool> bypassLogin          = false;
};
BOOST_DESCRIBE_STRUCT(Devel, (), (cacheBusterOn, sqlCache, simulateMachine, setMachineCheck, redirectIfPythonFail, bypassLogin))

enum class MType {
	WM,
	Sew,
	_4Assi,
};
BOOST_DESCRIBE_ENUM(MType, WM, Sew, _4Assi)

struct Config {
	DBConf            dbConf;
	NanoSpammerConfig spamConf;
	DBMaintainance    dbMaintainance;
	Http              http;
	Devel             devel;
	QString           editPassword;
	QString           resetPassword;
	MachineParameter  machineParameter;

	std::optional<MType>       mType             = MType::WM;
	std::optional<bool>        rebootEnabled     = true;
	std::optional<std::string> language          = "en_US";
	std::optional<int>         maxHeartbeatDelay = 5;
	std::optional<std::string> pythonLog         = "/root/wm/log/log";
	std::optional<int>         imageMagickVersion = 6;

	uint sessionLenght = 24 * 3600;
};
BOOST_DESCRIBE_STRUCT(Config, (),
                      (dbConf, spamConf, dbMaintainance, http, devel, editPassword, resetPassword, machineParameter,
                       sessionLenght, rebootEnabled, mType, language, maxHeartbeatDelay, pythonLog, imageMagickVersion))

Config defaultConfig();

} // namespace wm_config

namespace boost {
namespace json {

template <>
struct is_described_class<wm_config::Http> : std::true_type {};

} // namespace json
} // namespace boost
