#include "rbk/QStacker/qstacker.h"
#include <QByteArray>
#include <QDebug>
#include <zip.h>

// returns the (unzipped) content of a zipped file.
// zipped = content of the zipped file
QByteArray unzip1(const QByteArray& zipped) {
	zip_error_t error;
	zip_error_init(&error);
	auto            src = zip_source_buffer_create(zipped, (zip_uint64_t)zipped.size(), 1, &error);
	auto            za  = zip_open_from_source(src, 0, &error);
	struct zip_stat sb;
	for (auto i = 0; i < zip_get_num_entries(za, 0); i++) {
		if (zip_stat_index(za, i, 0, &sb) == 0) {
			//			printf("==================\n");
			//			auto len = strlen(sb.name);
			//			printf("Name: %s\n, ", sb.name);
			//			printf("Size: %lu\n, ", sb.size);
			//			printf("mtime: %u\n", (unsigned int)sb.mtime);
			//			fflush( stdout );
			auto zf = zip_fopen_index(za, i, 0);
			if (!zf) {
				qCritical().noquote() << "error iterating zip file" << QStacker16();
				return QByteArray();
			}

			QByteArray decompressed;
			decompressed.resize(static_cast<int>(sb.size));
			auto len = zip_fread(zf, decompressed.data(), sb.size);
			if (len < 0) {
				qCritical().noquote() << "error decompressing zip file" << QStacker16();
				return QByteArray();
			}
			zip_fclose(zf);
			// nothing to free as the lib is buggy and tries to deallocate the
			// original buffer -.-
			return decompressed;
		}
	}
	// some error
	qDebug().noquote() << "something strange with that zip file" << QStacker16();
	return QByteArray();
}
