#include "apptranslator.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QString>
#include <QTranslator>

namespace {

struct TranslationEntry {
    const char *context;
    const char *source;
    const char *italian;
    const char *french;
    const char *german;
};

constexpr TranslationEntry kTranslations[] = {
    {"MainWindow", "medAlert daily medication check", "controllo giornaliero medicine medAlert", "verification quotidienne des medicaments medAlert", "tagliche medAlert-Medikamentenprufung"},
    {"MainWindow", "Run medAlert daily at %1", "esegui medAlert ogni giorno alle %1", "executer medAlert chaque jour a %1", "medAlert taglich um %1 ausfuhren"},
    {"MainWindow", "The systemctl --user command did not complete.", "Il comando systemctl --user non ha completato l'operazione.", "La commande systemctl --user ne s'est pas terminee.", "Der Befehl systemctl --user wurde nicht abgeschlossen."},
    {"MainWindow", "Add medication", "Aggiungi farmaco", "Ajouter un medicament", "Medikament hinzufugen"},
    {"MainWindow", "Edit entry", "Modifica scheda", "Modifier la fiche", "Eintrag bearbeiten"},
    {"MainWindow", "Remove", "Rimuovi", "Supprimer", "Entfernen"},
    {"MainWindow", "Show selected", "Mostra selezionati", "Afficher la selection", "Auswahl anzeigen"},
    {"MainWindow", "Enable daily timer", "Attiva timer giornaliero", "Activer le minuteur quotidien", "Taglichen Timer aktivieren"},
    {"MainWindow", "Run check now", "Esegui controllo ora", "Executer la verification maintenant", "Prufung jetzt ausfuhren"},
    {"MainWindow", "Move up", "Sposta in alto", "Monter", "Nach oben"},
    {"MainWindow", "Move down", "Sposta in basso", "Descendre", "Nach unten"},
    {"MainWindow", "Settings", "Impostazioni", "Parametres", "Einstellungen"},
    {"MainWindow", "Application settings", "Impostazioni applicazione", "Parametres de l'application", "Anwendungseinstellungen"},
    {"MainWindow", "Language", "Lingua", "Langue", "Sprache"},
    {"MainWindow", "Update time", "Orario aggiornamento", "Heure de mise a jour", "Aktualisierungszeit"},
    {"MainWindow", "English", "Inglese", "Anglais", "Englisch"},
    {"MainWindow", "Italian", "Italiano", "Italien", "Italienisch"},
    {"MainWindow", "French", "Francese", "Francais", "Franzosisch"},
    {"MainWindow", "German", "Tedesco", "Allemand", "Deutsch"},
    {"MainWindow", "The application must be restarted to update every visible label after a language change.", "Dopo il cambio lingua potrebbe essere necessario riavviare l'app per aggiornare tutte le etichette visibili.", "Apres un changement de langue, il peut etre necessaire de redemarrer l'application pour actualiser toutes les etiquettes visibles.", "Nach einem Sprachwechsel kann ein Neustart der Anwendung erforderlich sein, um alle sichtbaren Beschriftungen zu aktualisieren."},
    {"MainWindow", "Medication", "Farmaco", "Medicament", "Medikament"},
    {"MainWindow", "Units/box", "Unita/conf.", "Unites/boite", "Einheiten/Packung"},
    {"MainWindow", "Units/day", "Unita/die", "Unites/jour", "Einheiten/Tag"},
    {"MainWindow", "Current units", "Stato corr.", "Unites actuelles", "Aktuelle Einheiten"},
    {"MainWindow", "Stock boxes", "Scorta conf.", "Boites en stock", "Packungen auf Lager"},
    {"MainWindow", "Alert threshold", "Soglia notif.", "Seuil d'alerte", "Warnschwelle"},
    {"MainWindow", "Standby", "Stand-by", "Veille", "Stand-by"},
    {"MainWindow", "Unable to load the JSON state file. It will be created on first save.", "Impossibile caricare il file JSON di stato. Verra creato al primo salvataggio.", "Impossible de charger le fichier d'etat JSON. Il sera cree lors du premier enregistrement.", "Die JSON-Statusdatei konnte nicht geladen werden. Sie wird beim ersten Speichern erstellt."},
    {"MainWindow", "Yes", "Si", "Oui", "Ja"},
    {"MainWindow", "No", "No", "Non", "Nein"},
    {"MainWindow", "JSON file: %1", "Archivio JSON: %1", "Fichier JSON : %1", "JSON-Datei: %1"},
    {"MainWindow", "Select a medication to move.", "Seleziona un farmaco da spostare.", "Selectionnez un medicament a deplacer.", "Wahlen Sie ein Medikament zum Verschieben aus."},
    {"MainWindow", "Select a medication to edit.", "Seleziona un farmaco da modificare.", "Selectionnez un medicament a modifier.", "Wahlen Sie ein Medikament zum Bearbeiten aus."},
    {"MainWindow", "Confirm removal", "Conferma rimozione", "Confirmer la suppression", "Entfernen bestatigen"},
    {"MainWindow", "Do you really want to remove this medication?", "Vuoi davvero rimuovere questo farmaco?", "Voulez-vous vraiment supprimer ce medicament ?", "Mochten Sie dieses Medikament wirklich entfernen?"},
    {"MainWindow", "The medication \"%1\" will be removed from the archive.", "Il farmaco \"%1\" verra eliminato dall'archivio.", "Le medicament \"%1\" sera supprime de l'archive.", "Das Medikament \"%1\" wird aus dem Bestand entfernt."},
    {"MainWindow", "Selected medications", "Farmaci selezionati", "Medicaments selectionnes", "Ausgewahlte Medikamente"},
    {"MainWindow", "Select one or more medications from the table.", "Seleziona uno o piu farmaci dalla tabella.", "Selectionnez un ou plusieurs medicaments dans le tableau.", "Wahlen Sie ein oder mehrere Medikamente aus der Tabelle aus."},
    {"MainWindow", "Timer activation", "Attivazione timer", "Activation du minuteur", "Timer-Aktivierung"},
    {"MainWindow", "The medalert.timer user timer has been installed and enabled for %1.", "Il timer utente medalert.timer e stato installato e abilitato alle %1.", "Le minuteur utilisateur medalert.timer a ete installe et active pour %1.", "Der Benutzertimer medalert.timer wurde installiert und fur %1 aktiviert."},
    {"MainWindow", "Check complete", "Controllo completato", "Verification terminee", "Prufung abgeschlossen"},
    {"MainWindow", "Nothing needs to be ordered.", "Non c'e nulla da ordinare.", "Rien ne doit etre commande.", "Es muss nichts bestellt werden."},
    {"MainWindow", "Low medication stock", "Scorte medicine in esaurimento", "Stock de medicaments faible", "Niedriger Medikamentenbestand"},
    {"MainWindow", "Unable to determine the user configuration directory.", "Impossibile determinare la directory di configurazione utente.", "Impossible de determiner le repertoire de configuration utilisateur.", "Das Benutzerkonfigurationsverzeichnis konnte nicht ermittelt werden."},
    {"MainWindow", "Unable to create directory %1.", "Impossibile creare la directory %1.", "Impossible de creer le repertoire %1.", "Verzeichnis %1 konnte nicht erstellt werden."},
    {"MainWindow", "Unable to write the user service file.", "Impossibile scrivere il file service utente.", "Impossible d'ecrire le fichier de service utilisateur.", "Die Benutzer-Service-Datei konnte nicht geschrieben werden."},
    {"MainWindow", "Unable to write the user timer file.", "Impossibile scrivere il file timer utente.", "Impossible d'ecrire le fichier de minuteur utilisateur.", "Die Benutzer-Timer-Datei konnte nicht geschrieben werden."},
    {"MainWindow", "Unable to save application settings.", "Impossibile salvare le impostazioni dell'applicazione.", "Impossible d'enregistrer les parametres de l'application.", "Die Anwendungseinstellungen konnten nicht gespeichert werden."},
    {"MainWindow", "Medication entry", "Scheda farmaco", "Fiche medicament", "Medikamenteneintrag"},
    {"MainWindow", "Medication on standby", "Farmaco in stand-by", "Medicament en veille", "Medikament im Stand-by"},
    {"MainWindow", "Medication name", "Nome farmaco", "Nom du medicament", "Medikamentenname"},
    {"MainWindow", "Units per box", "Unita per confezione", "Unites par boite", "Einheiten pro Packung"},
    {"MainWindow", "Units taken per day", "Unita assunte al giorno", "Unites prises par jour", "Einheiten pro Tag"},
    {"MainWindow", "Current state (units)", "Stato corrente (unita)", "Etat actuel (unites)", "Aktueller Stand (Einheiten)"},
    {"MainWindow", "Stock (boxes)", "Magazzino (confezioni)", "Stock (boites)", "Lagerbestand (Packungen)"},
    {"MainWindow", "Notification threshold (units)", "Soglia notifica (unita)", "Seuil de notification (unites)", "Benachrichtigungsschwelle (Einheiten)"},
    {"MainWindow", "Medication name is required.", "Il nome del farmaco e obbligatorio.", "Le nom du medicament est obligatoire.", "Der Medikamentenname ist erforderlich."},
    {"NotificationService", "Low medication stock", "Scorte medicine in esaurimento", "Stock de medicaments faible", "Niedriger Medikamentenbestand"},
    {"MedicineStore", "%1: %2 units left, stock depleted (threshold %3)", "%1: restano %2 unita, magazzino esaurito (soglia %3)", "%1 : il reste %2 unites, stock epuise (seuil %3)", "%1: %2 Einheiten ubrig, Lager erschopft (Schwelle %3)"},
    {"main", "Unable to load medicines JSON state.", "Impossibile caricare lo stato JSON delle medicine.", "Impossible de charger l'etat JSON des medicaments.", "Der JSON-Status der Medikamente konnte nicht geladen werden."},
    {"main", "Unable to deliver low-stock notification.", "Impossibile inviare la notifica di scorta in esaurimento.", "Impossible d'envoyer la notification de stock faible.", "Die Benachrichtigung uber niedrigen Bestand konnte nicht gesendet werden."}
};

} // namespace

namespace {

QTranslator *g_qtTranslator = nullptr;
AppTranslator *g_appTranslator = nullptr;
QString g_currentLanguageCode = QStringLiteral("en");

} // namespace

AppTranslator::AppTranslator(const QString &languageCode, QObject *parent)
    : QTranslator(parent)
{
    m_languageCode = normalizedLanguageCode(languageCode);
}

QString AppTranslator::normalizedLanguageCode(const QString &languageCode)
{
    const QString normalized = languageCode.section('_', 0, 0).trimmed().toLower();
    if (normalized == QStringLiteral("it")
        || normalized == QStringLiteral("fr")
        || normalized == QStringLiteral("de")) {
        return normalized;
    }
    return QString();
}

QString AppTranslator::languageFromLocale(const QLocale &locale)
{
    return normalizedLanguageCode(locale.name());
}

QStringList AppTranslator::supportedLanguageCodes()
{
    return {QStringLiteral("en"), QStringLiteral("it"), QStringLiteral("fr"), QStringLiteral("de")};
}

bool AppTranslator::isEmpty() const
{
    return m_languageCode.isEmpty();
}

QString AppTranslator::translate(const char *context,
                                 const char *sourceText,
                                 const char *disambiguation,
                                 int n) const
{
    Q_UNUSED(disambiguation)
    Q_UNUSED(n)

    if (m_languageCode.isEmpty() || context == nullptr || sourceText == nullptr) {
        return QString();
    }

    for (const TranslationEntry &entry : kTranslations) {
        if (qstrcmp(entry.context, context) != 0 || qstrcmp(entry.source, sourceText) != 0) {
            continue;
        }

        if (m_languageCode == QStringLiteral("it")) {
            return QString::fromUtf8(entry.italian);
        }
        if (m_languageCode == QStringLiteral("fr")) {
            return QString::fromUtf8(entry.french);
        }
        return QString::fromUtf8(entry.german);
    }

    return QString();
}

QString effectiveLanguageCode(const QString &savedLanguageCode, const QLocale &fallbackLocale)
{
    const QString saved = savedLanguageCode.section('_', 0, 0).trimmed().toLower();
    if (saved == QStringLiteral("en")) {
        return saved;
    }

    const QString translated = AppTranslator::normalizedLanguageCode(saved);
    if (!translated.isEmpty()) {
        return translated;
    }

    const QString fallback = AppTranslator::languageFromLocale(fallbackLocale);
    if (!fallback.isEmpty()) {
        return fallback;
    }

    return QStringLiteral("en");
}

QString currentAppLanguageCode()
{
    return g_currentLanguageCode;
}

void applyApplicationLanguage(QCoreApplication &app, const QString &languageCode)
{
    const QString effectiveCode = effectiveLanguageCode(languageCode);

    if (g_qtTranslator != nullptr) {
        app.removeTranslator(g_qtTranslator);
        delete g_qtTranslator;
        g_qtTranslator = nullptr;
    }

    if (g_appTranslator != nullptr) {
        app.removeTranslator(g_appTranslator);
        delete g_appTranslator;
        g_appTranslator = nullptr;
    }

    if (effectiveCode != QStringLiteral("en")) {
        auto *qtTranslator = new QTranslator();
        if (qtTranslator->load(QLocale(effectiveCode),
                               QStringLiteral("qtbase"),
                               QStringLiteral("_"),
                               QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            app.installTranslator(qtTranslator);
            g_qtTranslator = qtTranslator;
        } else {
            delete qtTranslator;
        }

        auto *appTranslator = new AppTranslator(effectiveCode);
        if (!appTranslator->isEmpty()) {
            app.installTranslator(appTranslator);
            g_appTranslator = appTranslator;
        } else {
            delete appTranslator;
        }
    }

    g_currentLanguageCode = effectiveCode;
}