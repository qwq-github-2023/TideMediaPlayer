#pragma once
#include <QString>
#include <QFile>
#include <QMessageBox>

#define TMH_UNKNOWN -1
#define TMH_VIDEO 0
#define TMH_IMAGE 1
#define TMH_AUDIO 2

class TideMediaHandle : public QFile
{
public:
	TideMediaHandle();
	~TideMediaHandle();

	bool loadMedia();
	int getMediaType();
private:
	int mediaType = TMH_UNKNOWN;
};

