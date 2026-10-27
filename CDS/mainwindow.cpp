#include "mainwindow.h"
#include <QtWidgets>
#include <QtSql>
// Modèle qui colore la colonne "Statut" du tableau des rendez-vous
class RdvModel : public QSqlQueryModel
{
public:
    explicit RdvModel(QObject *parent = 0) : QSqlQueryModel(parent) {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (index.column() == 7) {
            QString s = QSqlQueryModel::data(index, Qt::DisplayRole).toString();

            if (role == Qt::ForegroundRole) {
                if (s == "planifie") return QColor("#a16207");
                if (s == "traite")   return QColor("#15803d");
                if (s == "annule")   return QColor("#b91c1c");
            }
            if (role == Qt::BackgroundRole) {
                if (s == "planifie") return QColor("#fef3c7");
                if (s == "traite")   return QColor("#dcfce7");
                if (s == "annule")   return QColor("#fee2e2");
            }
            if (role == Qt::FontRole) {
                QFont f;
                f.setBold(true);
                return f;
            }
            if (role == Qt::TextAlignmentRole)
                return int(Qt::AlignCenter);
        }
        return QSqlQueryModel::data(index, role);
    }
};

// ---------- Fonctions utilitaires ----------

static QLabel *titre(const QString &texte)
{
    QLabel *l = new QLabel(texte);
    l->setObjectName("pageTitle");
    return l;
}

static void remplirCombo(QComboBox *combo, const QString &sql)
{
    QVariant ancien = combo->currentData();
    combo->clear();
    QSqlQuery q(sql);
    while (q.next())
        combo->addItem(q.value(1).toString(), q.value(0));
    int i = combo->findData(ancien);
    if (i >= 0)
        combo->setCurrentIndex(i);
}

static bool executer(QWidget *parent, QSqlQuery &q)
{
    if (!q.exec()) {
        QMessageBox::critical(parent, "Erreur SQL", q.lastError().text());
        return false;
    }
    return true;
}

// ---------- Construction de la fenêtre ----------

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Centre de services - Gestion des rendez-vous");
    resize(1150, 700);

    QWidget *central = new QWidget;
    QHBoxLayout *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    stack = new QStackedWidget;
    stack->setObjectName("content");
    stack->addWidget(pageRdv());
    stack->addWidget(pageTraitements());
    stack->addWidget(pageUsagers());
    stack->addWidget(pageAgents());
    stack->addWidget(pagePrestations());

    // Barre latérale
    QFrame *sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(220);
    QVBoxLayout *sl = new QVBoxLayout(sidebar);
    sl->setContentsMargins(12, 24, 12, 24);
    sl->setSpacing(6);

    QLabel *logo = new QLabel("Centre de\nServices");
    logo->setObjectName("logo");
    sl->addWidget(logo);
    sl->addSpacing(24);

    QStringList noms;
    noms << "Rendez-vous" << "Traitements" << "Usagers" << "Agents" << "Prestations";
    QButtonGroup *groupe = new QButtonGroup(this);
    for (int i = 0; i < noms.size(); ++i) {
        QPushButton *b = new QPushButton(noms.at(i));
        b->setObjectName("navBtn");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        groupe->addButton(b, i);
        sl->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, i]() {
            stack->setCurrentIndex(i);
            rafraichirListes();
        });
        if (i == 0)
            b->setChecked(true);
    }
    sl->addStretch();

    root->addWidget(sidebar);
    root->addWidget(stack, 1);
    setCentralWidget(central);
    // ombre douce sous chaque carte (formulaire)
    QList<QFrame  *>  cartes =  findChildren<QFrame  *>("card");
    for (int k=0;k<cartes.size();++k){
         QGraphicsDropShadowEffect *ombre=new QGraphicsDropShadowEffect(cartes.at(k));
         ombre->setBlurRadius(24);
         ombre->setOffset(0, 4);
         ombre->setColor(QColor(0,0,0,55));
         cartes.at(k)->setGraphicsEffect(ombre);

    }

    appliquerStyle();
    rafraichirListes();
}

QTableView *MainWindow::creerTable()
{
    QTableView *t = new QTableView;
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->verticalHeader()->hide();
    t->verticalHeader()->setDefaultSectionSize(36);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    return t;
}

// ---------- Page Rendez-vous ----------

QWidget *MainWindow::pageRdv()
{
    QWidget *page = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);
    lay->addWidget(titre("Gestion des rendez-vous"));

    QFrame *card = new QFrame;
    card->setObjectName("card");
    QGridLayout *g = new QGridLayout(card);
    g->setContentsMargins(16, 16, 16, 16);
    g->setHorizontalSpacing(14);
    g->setVerticalSpacing(8);

    comboUsager = new QComboBox;
    comboAgent = new QComboBox;
    comboPrestation = new QComboBox;
    dateRdv = new QDateEdit(QDate::currentDate());
    dateRdv->setCalendarPopup(true);
    dateRdv->setDisplayFormat("dd/MM/yyyy");
    dateRdv->setMinimumDate(QDate::currentDate());
    heureRdv = new QTimeEdit(QTime(8, 0));
    heureRdv->setDisplayFormat("HH:mm");

    g->addWidget(new QLabel("Usager"), 0, 0);
    g->addWidget(comboUsager, 1, 0);
    g->addWidget(new QLabel("Agent"), 0, 1);
    g->addWidget(comboAgent, 1, 1);
    g->addWidget(new QLabel("Prestation"), 0, 2);
    g->addWidget(comboPrestation, 1, 2);
    g->addWidget(new QLabel("Date"), 0, 3);
    g->addWidget(dateRdv, 1, 3);
    g->addWidget(new QLabel("Heure de début"), 0, 4);
    g->addWidget(heureRdv, 1, 4);

    QPushButton *btnPlan = new QPushButton("Planifier");
    btnPlan->setObjectName("btnPrimary");
    QPushButton *btnAnnul = new QPushButton("Annuler le rendez-vous sélectionné");
    btnAnnul->setObjectName("btnDanger");
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    btns->addWidget(btnPlan);
    btns->addWidget(btnAnnul);
    g->addLayout(btns, 2, 0, 1, 5);
    lay->addWidget(card);

    modelRdv = new RdvModel(this);
    tableRdv = creerTable();
    tableRdv->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableRdv->setModel(modelRdv);
    lay->addWidget(tableRdv, 1);

    connect(btnPlan, SIGNAL(clicked()), this, SLOT(planifierRdv()));
    connect(btnAnnul, SIGNAL(clicked()), this, SLOT(annulerRdv()));
    return page;
}

void MainWindow::planifierRdv()
{
    if (comboUsager->count() == 0 || comboAgent->count() == 0 || comboPrestation->count() == 0) {
        QMessageBox::warning(this, "Données manquantes",
            "Ajoutez d'abord au moins un usager, un agent et une prestation.");
        return;
    }

    int idUsager = comboUsager->currentData().toInt();
    int idAgent = comboAgent->currentData().toInt();
    int idPrest = comboPrestation->currentData().toInt();

    // 1) La durée dépend de la prestation
    QSqlQuery qd;
    qd.prepare("SELECT duree_minutes FROM prestation WHERE id_prestation = :p");
    qd.bindValue(":p", idPrest);
    if (!executer(this, qd) || !qd.next())
        return;
    int duree = qd.value(0).toInt();

    QTime debut = heureRdv->time();
    QTime fin = debut.addSecs(duree * 60);
    if (fin <= debut) {
        QMessageBox::warning(this, "Horaire invalide", "Le rendez-vous dépasserait minuit.");
        return;
    }
    QString sDebut = debut.toString("HH:mm:ss");
    QString sFin = fin.toString("HH:mm:ss");
    QString sDate = dateRdv->date().toString("yyyy-MM-dd");

    // 2) Un agent ne peut pas recevoir deux usagers en même temps
    //    (les rendez-vous annulés sont ignorés)
    QSqlQuery qc;
    qc.prepare("SELECT COUNT(*) FROM rendez_vous r "
               "JOIN prestation p ON p.id_prestation = r.id_prestation "
               "WHERE r.id_agent = :ag AND r.date_rdv = :d AND r.statut <> 'annule' "
               "AND r.heure_debut < :fin "
               "AND ADDTIME(r.heure_debut, SEC_TO_TIME(p.duree_minutes * 60)) > :debut");
    qc.bindValue(":ag", idAgent);
    qc.bindValue(":d", sDate);
    qc.bindValue(":fin", sFin);
    qc.bindValue(":debut", sDebut);
    if (!executer(this, qc) || !qc.next())
        return;
    if (qc.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Créneau indisponible",
            "Cet agent est déjà occupé sur ce créneau.\nChoisissez une autre heure ou un autre agent.");
        return;
    }

    // 3) Enregistrement
    QSqlQuery qi;
    qi.prepare("INSERT INTO rendez_vous (id_usager, id_agent, id_prestation, date_rdv, heure_debut, statut) "
               "VALUES (:u, :a, :p, :d, :h, 'planifie')");
    qi.bindValue(":u", idUsager);
    qi.bindValue(":a", idAgent);
    qi.bindValue(":p", idPrest);
    qi.bindValue(":d", sDate);
    qi.bindValue(":h", sDebut);
    if (executer(this, qi)) {
        rafraichirListes();
        QMessageBox::information(this, "Rendez-vous planifié",
            QString("Rendez-vous enregistré de %1 à %2.")
                .arg(debut.toString("HH:mm"), fin.toString("HH:mm")));
    }
}

void MainWindow::annulerRdv()
{
    QModelIndex idx = tableRdv->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "Sélection", "Sélectionnez d'abord un rendez-vous.");
        return;
    }
    int id = modelRdv->data(modelRdv->index(idx.row(), 0)).toInt();
    QString statut = modelRdv->data(modelRdv->index(idx.row(), 7)).toString();

    if (statut == "annule") {
        QMessageBox::information(this, "Information", "Ce rendez-vous est déjà annulé.");
        return;
    }
    if (statut == "traite") {
        QMessageBox::warning(this, "Impossible", "Un rendez-vous déjà traité ne peut pas être annulé.");
        return;
    }
    if (QMessageBox::question(this, "Confirmation", "Annuler ce rendez-vous ?") != QMessageBox::Yes)
        return;

    QSqlQuery q;
    q.prepare("UPDATE rendez_vous SET statut = 'annule' WHERE id_rdv = :id");
    q.bindValue(":id", id);
    if (executer(this, q))
        rafraichirListes();
}

// ---------- Page Traitements ----------

QWidget *MainWindow::pageTraitements()
{
    QWidget *page = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);
    lay->addWidget(titre("Suivi des demandes traitées"));

    QFrame *card = new QFrame;
    card->setObjectName("card");
    QGridLayout *g = new QGridLayout(card);
    g->setContentsMargins(16, 16, 16, 16);
    g->setHorizontalSpacing(14);
    g->setVerticalSpacing(8);

    comboRdvATraiter = new QComboBox;
    dateTraitement = new QDateEdit(QDate::currentDate());
    dateTraitement->setCalendarPopup(true);
    dateTraitement->setDisplayFormat("dd/MM/yyyy");
    comboResultat = new QComboBox;
    comboResultat->setEditable(true);
    comboResultat->addItems(QStringList() << "Dossier complet" << "Dossier incomplet"
                                          << "Demande refusée" << "Orienté vers un autre service");
    textObservation = new QTextEdit;
    textObservation->setFixedHeight(70);
    textObservation->setPlaceholderText("Observation (facultatif)");

    g->addWidget(new QLabel("Rendez-vous à traiter"), 0, 0, 1, 2);
    g->addWidget(comboRdvATraiter, 1, 0, 1, 2);
    g->addWidget(new QLabel("Date du traitement"), 0, 2);
    g->addWidget(dateTraitement, 1, 2);
    g->addWidget(new QLabel("Résultat"), 0, 3);
    g->addWidget(comboResultat, 1, 3);
    g->addWidget(new QLabel("Observation"), 2, 0, 1, 4);
    g->addWidget(textObservation, 3, 0, 1, 4);
    QPushButton *btnSave = new QPushButton("Enregistrer le traitement");
    btnSave->setObjectName("btnPrimary");
    g->addWidget(btnSave, 4, 0, 1, 4, Qt::AlignRight);
    lay->addWidget(card);

    modelTraitement = new QSqlQueryModel(this);
    tableTraitement = creerTable();
    tableTraitement->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableTraitement->setModel(modelTraitement);
    lay->addWidget(tableTraitement, 1);

    connect(btnSave, SIGNAL(clicked()), this, SLOT(enregistrerTraitement()));
    return page;
}

void MainWindow::enregistrerTraitement()
{
    if (comboRdvATraiter->count() == 0) {
        QMessageBox::information(this, "Information", "Aucun rendez-vous à traiter pour le moment.");
        return;
    }
    int idRdv = comboRdvATraiter->currentData().toInt();

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery q1;
    q1.prepare("INSERT INTO traitement (id_rdv, date_traitement, observation, resultat) "
               "VALUES (:r, :d, :o, :res)");
    q1.bindValue(":r", idRdv);
    q1.bindValue(":d", dateTraitement->date().toString("yyyy-MM-dd"));
    q1.bindValue(":o", textObservation->toPlainText().trimmed());
    q1.bindValue(":res", comboResultat->currentText().trimmed());

    QSqlQuery q2;
    q2.prepare("UPDATE rendez_vous SET statut = 'traite' WHERE id_rdv = :r");
    q2.bindValue(":r", idRdv);

    bool ok = q1.exec() && q2.exec();
    if (ok) {
        db.commit();
        textObservation->clear();
        rafraichirListes();
    } else {
        db.rollback();
        QString err = q1.lastError().isValid() ? q1.lastError().text() : q2.lastError().text();
        QMessageBox::critical(this, "Erreur SQL", err);
    }
}

// ---------- Page Usagers ----------

QWidget *MainWindow::pageUsagers()
{
    QWidget *page = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);
    lay->addWidget(titre("Usagers"));

    QFrame *card = new QFrame;
    card->setObjectName("card");
    QHBoxLayout *form = new QHBoxLayout(card);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(12);
    editUsagerNom = new QLineEdit;
    editUsagerNom->setPlaceholderText("Nom complet");
    editUsagerTel = new QLineEdit;
    editUsagerTel->setPlaceholderText("Téléphone");
    QPushButton *btnAdd = new QPushButton("Ajouter");
    btnAdd->setObjectName("btnPrimary");
    QPushButton *btnDel = new QPushButton("Supprimer la sélection");
    btnDel->setObjectName("btnDanger");
    form->addWidget(editUsagerNom, 2);
    form->addWidget(editUsagerTel, 1);
    form->addWidget(btnAdd);
    form->addWidget(btnDel);
    lay->addWidget(card);

    modelUsager = new QSqlTableModel(this);
    modelUsager->setTable("usager");
    modelUsager->setEditStrategy(QSqlTableModel::OnFieldChange);
    tableUsager = creerTable();
    tableUsager->setModel(modelUsager);
    lay->addWidget(tableUsager, 1);

    QLabel *hint = new QLabel("Double-cliquez sur une cellule pour la modifier.");
    hint->setObjectName("hint");
    lay->addWidget(hint);

    connect(btnAdd, SIGNAL(clicked()), this, SLOT(ajouterUsager()));
    connect(btnDel, SIGNAL(clicked()), this, SLOT(supprimerUsager()));
    return page;
}

void MainWindow::ajouterUsager()
{
    QString nom = editUsagerNom->text().trimmed();
    QString tel = editUsagerTel->text().trimmed();
    if (nom.isEmpty() || tel.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Renseignez le nom et le téléphone.");
        return;
    }
    QSqlQuery q;
    q.prepare("INSERT INTO usager (nom, telephone) VALUES (:n, :t)");
    q.bindValue(":n", nom);
    q.bindValue(":t", tel);
    if (executer(this, q)) {
        editUsagerNom->clear();
        editUsagerTel->clear();
        rafraichirListes();
    }
}

void MainWindow::supprimerUsager()
{
    supprimerLigne(tableUsager, modelUsager);
}

// ---------- Page Agents ----------

QWidget *MainWindow::pageAgents()
{
    QWidget *page = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);
    lay->addWidget(titre("Agents"));

    QFrame *card = new QFrame;
    card->setObjectName("card");
    QHBoxLayout *form = new QHBoxLayout(card);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(12);
    editAgentNom = new QLineEdit;
    editAgentNom->setPlaceholderText("Nom complet");
    editAgentFonction = new QLineEdit;
    editAgentFonction->setPlaceholderText("Fonction");
    QPushButton *btnAdd = new QPushButton("Ajouter");
    btnAdd->setObjectName("btnPrimary");
    QPushButton *btnDel = new QPushButton("Supprimer la sélection");
    btnDel->setObjectName("btnDanger");
    form->addWidget(editAgentNom, 2);
    form->addWidget(editAgentFonction, 1);
    form->addWidget(btnAdd);
    form->addWidget(btnDel);
    lay->addWidget(card);

    modelAgent = new QSqlTableModel(this);
    modelAgent->setTable("agent");
    modelAgent->setEditStrategy(QSqlTableModel::OnFieldChange);
    tableAgent = creerTable();
    tableAgent->setModel(modelAgent);
    lay->addWidget(tableAgent, 1);

    QLabel *hint = new QLabel("Double-cliquez sur une cellule pour la modifier.");
    hint->setObjectName("hint");
    lay->addWidget(hint);

    connect(btnAdd, SIGNAL(clicked()), this, SLOT(ajouterAgent()));
    connect(btnDel, SIGNAL(clicked()), this, SLOT(supprimerAgent()));
    return page;
}

void MainWindow::ajouterAgent()
{
    QString nom = editAgentNom->text().trimmed();
    QString fonction = editAgentFonction->text().trimmed();
    if (nom.isEmpty() || fonction.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Renseignez le nom et la fonction.");
        return;
    }
    QSqlQuery q;
    q.prepare("INSERT INTO agent (nom, fonction) VALUES (:n, :f)");
    q.bindValue(":n", nom);
    q.bindValue(":f", fonction);
    if (executer(this, q)) {
        editAgentNom->clear();
        editAgentFonction->clear();
        rafraichirListes();
    }
}

void MainWindow::supprimerAgent()
{
    supprimerLigne(tableAgent, modelAgent);
}

// ---------- Page Prestations ----------

QWidget *MainWindow::pagePrestations()
{
    QWidget *page = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);
    lay->addWidget(titre("Prestations"));

    QFrame *card = new QFrame;
    card->setObjectName("card");
    QHBoxLayout *form = new QHBoxLayout(card);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(12);
    editPrestLibelle = new QLineEdit;
    editPrestLibelle->setPlaceholderText("Libellé de la prestation");
    spinDuree = new QSpinBox;
    spinDuree->setRange(5, 480);
    spinDuree->setValue(15);
    spinDuree->setSuffix(" min");
    spinTarif = new QDoubleSpinBox;
    spinTarif->setRange(0, 10000000);
    spinTarif->setDecimals(2);
    spinTarif->setSuffix(" Ar");
    QPushButton *btnAdd = new QPushButton("Ajouter");
    btnAdd->setObjectName("btnPrimary");
    QPushButton *btnDel = new QPushButton("Supprimer la sélection");
    btnDel->setObjectName("btnDanger");
    form->addWidget(editPrestLibelle, 2);
    form->addWidget(spinDuree);
    form->addWidget(spinTarif);
    form->addWidget(btnAdd);
    form->addWidget(btnDel);
    lay->addWidget(card);

    modelPrestation = new QSqlTableModel(this);
    modelPrestation->setTable("prestation");
    modelPrestation->setEditStrategy(QSqlTableModel::OnFieldChange);
    tablePrestation = creerTable();
    tablePrestation->setModel(modelPrestation);
    lay->addWidget(tablePrestation, 1);

    QLabel *hint = new QLabel("Double-cliquez sur une cellule pour la modifier. "
                              "La durée de la prestation détermine la durée des rendez-vous.");
    hint->setObjectName("hint");
    lay->addWidget(hint);

    connect(btnAdd, SIGNAL(clicked()), this, SLOT(ajouterPrestation()));
    connect(btnDel, SIGNAL(clicked()), this, SLOT(supprimerPrestation()));
    return page;
}

void MainWindow::ajouterPrestation()
{
    QString libelle = editPrestLibelle->text().trimmed();
    if (libelle.isEmpty()) {
        QMessageBox::warning(this, "Champ manquant", "Renseignez le libellé de la prestation.");
        return;
    }
    QSqlQuery q;
    q.prepare("INSERT INTO prestation (libelle, duree_minutes, tarif) VALUES (:l, :d, :t)");
    q.bindValue(":l", libelle);
    q.bindValue(":d", spinDuree->value());
    q.bindValue(":t", spinTarif->value());
    if (executer(this, q)) {
        editPrestLibelle->clear();
        rafraichirListes();
    }
}

void MainWindow::supprimerPrestation()
{
    supprimerLigne(tablePrestation, modelPrestation);
}

// ---------- Fonctions communes ----------

void MainWindow::supprimerLigne(QTableView *table, QSqlTableModel *model)
{
    QModelIndex idx = table->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "Sélection", "Sélectionnez d'abord une ligne.");
        return;
    }
    if (QMessageBox::question(this, "Confirmation", "Supprimer la ligne sélectionnée ?") != QMessageBox::Yes)
        return;

    if (!model->removeRow(idx.row())) {
        QMessageBox::warning(this, "Suppression impossible",
            "Cet élément est déjà utilisé par des rendez-vous.\n\n" + model->lastError().text());
    }
    rafraichirListes();
}

void MainWindow::majModeles()
{
    modelUsager->select();
    modelUsager->setHeaderData(1, Qt::Horizontal, "Nom");
    modelUsager->setHeaderData(2, Qt::Horizontal, "Téléphone");
    tableUsager->setColumnHidden(0, true);

    modelAgent->select();
    modelAgent->setHeaderData(1, Qt::Horizontal, "Nom");
    modelAgent->setHeaderData(2, Qt::Horizontal, "Fonction");
    tableAgent->setColumnHidden(0, true);

    modelPrestation->select();
    modelPrestation->setHeaderData(1, Qt::Horizontal, "Libellé");
        modelPrestation->setHeaderData(2, Qt::Horizontal, "Durée (minutes)");
        modelPrestation->setHeaderData(3, Qt::Horizontal, "Tarif");
        tablePrestation->setColumnHidden(0, true);

        modelRdv->setQuery(
            "SELECT r.id_rdv, u.nom, a.nom, p.libelle, "
            "DATE_FORMAT(r.date_rdv, '%d/%m/%Y'), "
            "TIME_FORMAT(r.heure_debut, '%H:%i'), "
            "TIME_FORMAT(ADDTIME(r.heure_debut, SEC_TO_TIME(p.duree_minutes * 60)), '%H:%i'), "
            "r.statut "
            "FROM rendez_vous r "
            "JOIN usager u ON u.id_usager = r.id_usager "
            "JOIN agent a ON a.id_agent = r.id_agent "
            "JOIN prestation p ON p.id_prestation = r.id_prestation "
            "ORDER BY r.date_rdv DESC, r.heure_debut DESC");
        modelRdv->setHeaderData(1, Qt::Horizontal, "Usager");
        modelRdv->setHeaderData(2, Qt::Horizontal, "Agent");
        modelRdv->setHeaderData(3, Qt::Horizontal, "Prestation");
        modelRdv->setHeaderData(4, Qt::Horizontal, "Date");
        modelRdv->setHeaderData(5, Qt::Horizontal, "Début");
        modelRdv->setHeaderData(6, Qt::Horizontal, "Fin");
        modelRdv->setHeaderData(7, Qt::Horizontal, "Statut");
        tableRdv->setColumnHidden(0, true);

        modelTraitement->setQuery(
            "SELECT t.id_traitement, DATE_FORMAT(t.date_traitement, '%d/%m/%Y'), "
            "u.nom, p.libelle, a.nom, t.resultat, t.observation "
            "FROM traitement t "
            "JOIN rendez_vous r ON r.id_rdv = t.id_rdv "
            "JOIN usager u ON u.id_usager = r.id_usager "
            "JOIN agent a ON a.id_agent = r.id_agent "
            "JOIN prestation p ON p.id_prestation = r.id_prestation "
            "ORDER BY t.id_traitement DESC");
        modelTraitement->setHeaderData(1, Qt::Horizontal, "Date");
        modelTraitement->setHeaderData(2, Qt::Horizontal, "Usager");
        modelTraitement->setHeaderData(3, Qt::Horizontal, "Prestation");
        modelTraitement->setHeaderData(4, Qt::Horizontal, "Agent");
        modelTraitement->setHeaderData(5, Qt::Horizontal, "Résultat");
        modelTraitement->setHeaderData(6, Qt::Horizontal, "Observation");
        tableTraitement->setColumnHidden(0, true);
    }

    void MainWindow::rafraichirListes()
    {
        majModeles();

        remplirCombo(comboUsager, "SELECT id_usager, nom FROM usager ORDER BY nom");
        remplirCombo(comboAgent, "SELECT id_agent, nom FROM agent ORDER BY nom");
        remplirCombo(comboPrestation,
            "SELECT id_prestation, CONCAT(libelle, ' (', duree_minutes, ' min)') "
            "FROM prestation ORDER BY libelle");
        remplirCombo(comboRdvATraiter,
            "SELECT r.id_rdv, CONCAT(DATE_FORMAT(r.date_rdv, '%d/%m/%Y'), ' ', "
            "TIME_FORMAT(r.heure_debut, '%H:%i'), ' - ', u.nom, ' (', p.libelle, ')') "
            "FROM rendez_vous r "
            "JOIN usager u ON u.id_usager = r.id_usager "
            "JOIN prestation p ON p.id_prestation = r.id_prestation "
            "WHERE r.statut = 'planifie' ORDER BY r.date_rdv, r.heure_debut");
    }

    // ---------- Style (QSS) ----------

    void MainWindow::appliquerStyle()
    {
        setStyleSheet(
            // Fond de la page : beige
            "QStackedWidget#content { background: #f2ebe3; }"

            // Barre latérale : marron foncé
            "QFrame#sidebar { background: #3e2c23; }"
            "QLabel#logo { color: #f5ede3; font-size: 22px; font-weight: bold; }"
            "QPushButton#navBtn { background: transparent; color: #d6c3b0; text-align: left;"
            "  padding: 13px 18px; border: none; border-radius: 10px; font-size: 14px; }"
            "QPushButton#navBtn:hover { background: #5a4234; color: white; }"
            "QPushButton#navBtn:checked { background: #8b5e3c; color: white; font-weight: bold; }"

            // Titres, textes et cartes
            "QLabel#pageTitle { font-size: 26px; font-weight: bold; color: #3e2c23; }"
            "QLabel#hint { color: #78716c; font-size: 12px; }"
            "QFrame#card { background: #fbf8f4; border: 1px solid #e0d5c8; border-radius: 14px; }"
            "QFrame#card QLabel { color: #57534e; font-weight: bold; border: none; }"

            // Champs de saisie
            "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit, QTimeEdit, QTextEdit {"
            "  background: #faf7f2; border: 1px solid #cfc3b5; border-radius: 8px;"
            "  padding: 7px; min-height: 24px; color: #3e2c23; }"
            "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,"
            "QDateEdit:focus, QTimeEdit:focus, QTextEdit:focus {"
            "  border: 2px solid #8b5e3c; background: white; }"

            // Boutons
            "QPushButton#btnPrimary { background: #8b5e3c; color: white; border: none;"
            "  border-radius: 8px; padding: 10px 22px; font-weight: bold; }"
            "QPushButton#btnPrimary:hover { background: #6f4a2f; }"
            "QPushButton#btnPrimary:pressed { background: #5a3b25; }"
            "QPushButton#btnDanger { background: #b4483b; color: white; border: none;"
            "  border-radius: 8px; padding: 10px 22px; font-weight: bold; }"
            "QPushButton#btnDanger:hover { background: #963a2f; }"
            "QPushButton#btnDanger:pressed { background: #7a2e25; }"

            // Tableaux : en-tête gris foncé, sélection beige
            "QTableView { background: #fbf8f4; alternate-background-color: #f4eee6;"
            "  border: 1px solid #e0d5c8; border-radius: 10px; color: #3e2c23;"
            "  selection-background-color: #e7d8c6; selection-color: #3e2c23; }"
            "QHeaderView::section { background: #57534e; color: white; padding: 10px;"
            "  border: none; font-weight: bold; }"

            // Barres de défilement
            "QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }"
            "QScrollBar::handle:vertical { background: #cbbfb0; border-radius: 4px; min-height: 30px; }"
            "QScrollBar::handle:vertical:hover { background: #a8998a; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }"
            "QScrollBar::handle:horizontal { background: #cbbfb0; border-radius: 4px; min-width: 30px; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        );
    }
