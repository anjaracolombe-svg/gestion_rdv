#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;
class QTableView;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QDateEdit;
class QTimeEdit;
class QTextEdit;
class QSqlTableModel;
class QSqlQueryModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

private slots:
    void ajouterUsager();
    void supprimerUsager();
    void ajouterAgent();
    void supprimerAgent();
    void ajouterPrestation();
    void supprimerPrestation();
    void planifierRdv();
    void annulerRdv();
    void enregistrerTraitement();
    void rafraichirListes();

private:
    QWidget *pageRdv();
    QWidget *pageTraitements();
    QWidget *pageUsagers();
    QWidget *pageAgents();
    QWidget *pagePrestations();
    QTableView *creerTable();
    void supprimerLigne(QTableView *table, QSqlTableModel *model);
    void majModeles();
    void appliquerStyle();

    QStackedWidget *stack;

    QSqlTableModel *modelUsager;
    QSqlTableModel *modelAgent;
    QSqlTableModel *modelPrestation;
    QSqlQueryModel *modelRdv;
    QSqlQueryModel *modelTraitement;

    QTableView *tableUsager;
    QTableView *tableAgent;
    QTableView *tablePrestation;
    QTableView *tableRdv;
    QTableView *tableTraitement;

    QLineEdit *editUsagerNom;
    QLineEdit *editUsagerTel;
    QLineEdit *editAgentNom;
    QLineEdit *editAgentFonction;
    QLineEdit *editPrestLibelle;
    QSpinBox *spinDuree;
    QDoubleSpinBox *spinTarif;

    QComboBox *comboUsager;
    QComboBox *comboAgent;
    QComboBox *comboPrestation;
    QComboBox *comboRdvATraiter;
    QComboBox *comboResultat;
    QDateEdit *dateRdv;
    QDateEdit *dateTraitement;
    QTimeEdit *heureRdv;
    QTextEdit *textObservation;
};

#endif // MAINWINDOW_H
