#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void showError(const QString &message);

public slots:
    void openFile();
    void saveToFile();

protected:
    void setupActions();

    QString selectFile();
    void openFile(const QString &fileName);
    void saveToFile(const QString &fileName, const QString &content);

private:
    Ui::MainWindow *ui;
    QString openedFile;
};
#endif // MAINWINDOW_H
