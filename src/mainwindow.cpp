#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "parser.hpp"
#include "parseresult.h"

#include <tuple>
#include <sstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showError(const QString &message)
{
    qDebug() << message;
    QMessageBox::information(this, "Message", message);
}

void MainWindow::openFile()
{
    openFile(selectFileToOpen());
}

void MainWindow::saveToFile()
{
    saveToFile(selectFileToSave(), ui->fileContentEdit->toPlainText());
}

void MainWindow::parse()
{
    const QString &source = ui->fileContentEdit->toPlainText();
    if (source.size() == 0) {
        return;
    }
    const char* begin = qPrintable(source);
    const char* end = begin + source.size();

    ParseResult result = parse_source(begin, end);

    StringTable &table = result.table;
    QString output = QString::fromStdString(result.output);
    QTextStream stream(&output);
    bool rowComma = false;
    stream << "{\n";
    for (auto &row : table) {
        if (rowComma) {
            stream << ",\n";
        }
        rowComma = true;
        bool cellComma = false;
        stream << "  {\n";
        for (auto &cell : row) {
            if (cellComma) {
                stream << ",\n";
            }
            cellComma = true;
            stream << "    \"" << cell << "\"";
        }
        stream << "\n  }";
    }
    stream << "};\n";

    ui->parsedResultsEdit->setPlainText(output);
}

void MainWindow::setupActions()
{
    QAction *openFile = ui->toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon),
                           "Open File");
    QAction *saveFile = ui->toolBar->addAction(style()->standardIcon(QStyle::SP_DriveFDIcon),
                           "Save File");
    QAction *parse = ui->toolBar->addAction(style()->standardIcon(QStyle::SP_ArrowRight),
                           "Parse");

    openFile->setShortcut(QKeySequence::Open);
    saveFile->setShortcut(QKeySequence::Save);
    parse->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter));
    //Ignore argument from signal
    connect(openFile, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(saveFile, SIGNAL(triggered(bool)), this, SLOT(saveToFile()));
    connect(parse, SIGNAL(triggered(bool)), this, SLOT(parse()));
}

QString MainWindow::selectFileToOpen()
{
    return QFileDialog::getOpenFileName(this, tr("Open File"),
                                        QDir::currentPath(),
                                        tr("All files (*.*)"));
}

QString MainWindow::selectFileToSave()
{
    return QFileDialog::getSaveFileName(this, tr("Save to File"),
                                        QDir::currentPath(),
                                        tr("All files (*.*)"));
}

void MainWindow::openFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.exists()) {
        showError(QString("File %1 not found.").arg(fileName));
        return;
    }
    QString line;
    ui->fileContentEdit->clear();
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            line = stream.readLine();
            ui->fileContentEdit->appendPlainText(line + '\n');
        }
    } else {
        showError(QString("Can't read file %1").arg(fileName));
        return;
    }
    file.close();
}

void MainWindow::saveToFile(const QString &fileName, const QString &content)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream stream(&file);
        stream << content;
    } else {
        showError(QString("Can't write to file %1").arg(fileName));
        return;
    }
    file.close();
}
