#include "mainwindow.h"
#include "./ui_mainwindow.h"

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
    openFile(selectFile());
}

void MainWindow::saveToFile()
{
    if (openedFile.size() == 0) {
        return;
    }
    saveToFile(openedFile, ui->fileContentEdit->toPlainText());
}

void MainWindow::setupActions()
{
    QAction *openFile = ui->toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon),
                           "Open File");
    QAction *saveFile = ui->toolBar->addAction(style()->standardIcon(QStyle::SP_DriveFDIcon),
                           "Save File");
    QAction *parse = ui->toolBar->addAction(style()->standardIcon(QStyle::SP_DialogApplyButton),
                           "Parse");

    openFile->setShortcut(QKeySequence::Open);
    saveFile->setShortcut(QKeySequence::Save);
    parse->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter));
    //Ignore argument from signal
    connect(openFile, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(saveFile, SIGNAL(triggered(bool)), this, SLOT(saveToFile()));
}

QString MainWindow::selectFile()
{
    return QFileDialog::getOpenFileName(this, tr("Open File"),
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
        openedFile = fileName;
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
