#ifndef PARSERESULT_H
#define PARSERESULT_H

#include <QtCore>

using StringTable = QLinkedList<QLinkedList<QString>>;

struct ParseResult {
    bool ok;
    QString output;
    StringTable table;
};

#endif // PARSERESULT_H
