#ifndef UADECODER_H
#define UADECODER_H

#include <QString>
/*
 Use like that

 //init the semaphore in case you need concurrency control
 std::counting_semaphore<16> uaSem(0);

 ....

        UaDecoder ua;
        if (uaSem.try_acquire()) {
                ua.decode(dk.userAgent, conf().uaDecoder.path);
                uaSem.release();
        }

*/

/*
Full list as of 2022 April is
camera
tv
phablet
portable media player
peripheral
wearable
tablet
car browser
console
desktop
smartphone
*/

//https://s22.trott.pw/dev_wiki/index.php?title=Table/externalAgencies/adUnitIdBlacklistedMarket
class DeviceRedux {
      public:
	enum Device {
		NA         = 0,
		smartphone = 1,
		tablet     = 2,
		desktop    = 3
	} device      = NA;
	DeviceRedux() = default;
	DeviceRedux(const QString& full);
	void operator()(const QString& full);

	operator QString() const;
};

class UaDecoder {
      public:
	UaDecoder(const QString& userAgent, const QString& decoderUrl);
	UaDecoder() = default;
	bool               decode(const QString& userAgent, const QString& decoderUrl);
	static std::string getHtml();

	bool        decoded = false;
	bool        ok      = false;
	QString     osName;
	QString     osVersion;
	float       browserVersion = 0;
	QString     browserName;
	QString     device;
	DeviceRedux deviceRDX;
	QString     brand;
	QString     bot;
	QString     enabled;
	bool        isMobile() const;
};

#endif // UADECODER_H
