CREATE TABLE usager (
    id_usager INT AUTO_INCREMENT PRIMARY KEY,
    nom VARCHAR(100) NOT NULL,
    telephone VARCHAR(20) NOT NULL
);

CREATE TABLE agent (
    id_agent INT AUTO_INCREMENT PRIMARY KEY,
    nom VARCHAR(100) NOT NULL,
    fonction VARCHAR(100) NOT NULL
);

CREATE TABLE prestation (
    id_prestation INT AUTO_INCREMENT PRIMARY KEY,
    libelle VARCHAR(150) NOT NULL,
    duree_minutes INT NOT NULL,
    tarif DECIMAL(10,2) NOT NULL
);

CREATE TABLE rendez_vous (
    id_rdv INT AUTO_INCREMENT PRIMARY KEY,
    id_usager INT NOT NULL,
    id_agent INT NOT NULL,
    id_prestation INT NOT NULL,
    date_rdv DATE NOT NULL,
    heure_debut TIME NOT NULL,
    statut ENUM('planifie', 'annule', 'termine')
        NOT NULL DEFAULT 'planifie',
    FOREIGN KEY (id_usager) REFERENCES usager(id_usager),
    FOREIGN KEY (id_agent) REFERENCES agent(id_agent),
    FOREIGN KEY (id_prestation)
        REFERENCES prestation(id_prestation)
);

CREATE TABLE traitement (
    id_traitement INT AUTO_INCREMENT PRIMARY KEY,
    id_rdv INT NOT NULL UNIQUE,
    date_traitement DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    observation TEXT,
    resultat VARCHAR(100),
    FOREIGN KEY (id_rdv) REFERENCES rendez_vous(id_rdv)
);