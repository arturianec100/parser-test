#ifndef PARSERESULT_H
#define PARSERESULT_H

#include <QtCore>

using StringTable = QVector<QVector<QString>>;
using StringRow = QVector<QString>;

struct ParseResult {
    StringTable table;
    std::string output;
    bool ok = false;
    size_t tableBeginIdx = 0;
    size_t tableEndIdx = 0;
};

#endif // PARSERESULT_H
