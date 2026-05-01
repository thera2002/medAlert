# medAlert

Application de bureau en C++ et Qt6 pour suivre les medicaments a la maison, mettre a jour le stock chaque jour et avertir lorsqu'un medicament arrive a epuisement.

Documentation disponible :

- Anglais : `README.md`
- Italien : `README-it.md`
- Francais : `README-fr.md`
- Allemand : `README-de.md`

## Fonctions principales

- une fiche par medicament avec nom, unites par boite et dosage quotidien
- etat courant exprime en nombre d'unites restantes
- stock exprime en nombre de boites de reserve
- possibilite de mettre un medicament en veille, en suspendant la mise a jour automatique de 23:00
- mise a jour automatique une fois par jour a 23:00
- rechargement automatique depuis le stock lorsque les unites courantes sont epuisees
- seuil d'alerte configurable pour chaque medicament, avec une valeur par defaut de 10 unites
- notification lorsque les unites restantes passent sous le seuil configure et qu'il ne reste plus de boites de reserve
- selection multiple de lignes avec le comportement standard de la table Qt, y compris Maj+clic
- boutons fleche pour deplacer la ligne selectionnee d'une position vers le haut ou vers le bas
- bouton qui affiche les noms des medicaments selectionnes dans une fenetre modale, un par ligne
- bouton `Settings` qui ouvre une fenetre avec le choix de la langue et de l'heure de mise a jour quotidienne
- la verification manuelle envoie toujours une notification, meme lorsqu'il n'y a rien a commander, afin de tester le systeme
- mode headless `--check` pour executer la verification quotidienne sans ouvrir l'interface graphique
- bouton GUI pour installer et activer automatiquement un minuteur `systemd --user` a l'heure configuree
- persistance complete dans un fichier JSON mis a jour apres chaque modification
- interface source en anglais avec localisation runtime en italien, francais et allemand selon la langue du systeme

## Notifications sur GNOME 3 et Wayland

La solution la plus universelle sur Linux desktop est l'interface D-Bus `org.freedesktop.Notifications`.
Qt6 l'utilise via le module `Qt6::DBus` et cela fonctionne bien sur GNOME 3, y compris en session Wayland.

Si le service de notification n'est pas disponible, l'application utilise une fenetre d'avertissement Qt comme solution de secours.

## Fichier d'etat JSON

Le fichier JSON est enregistre sous `QStandardPaths::GenericDataLocation`, par exemple :

```text
~/.local/share/medAlert/medicines.json
```

Il contient :

- `lastProcessedDate` : derniere date pour laquelle la consommation quotidienne a deja ete appliquee
- `medicines` : liste des medicaments, y compris l'indicateur `standby`, le seuil d'alerte et l'etat courant du stock

L'application enregistre aussi les preferences utilisateur dans :

```text
~/.local/share/medAlert/settings.json
```

Ce fichier contient :

- `preferredLanguage` : langue selectionnee pour l'interface
- `updateTime` : heure de mise a jour quotidienne au format `HH:mm`

## Localisation

La langue source de l'application est l'anglais.

Au demarrage l'application charge automatiquement une traduction si la langue du systeme correspond a l'une de celles-ci :

- italien
- francais
- allemand

Si aucune traduction prise en charge n'est disponible, l'interface reste en anglais.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Lancement

```bash
./build/medAlert
```

## Verification headless

```bash
./build/medAlert --check
```

Le mode `--check` lit le fichier JSON, applique la mise a jour quotidienne et envoie une notification uniquement lorsqu'un ou plusieurs medicaments sont sous le seuil.

## Minuteur systemd utilisateur

L'application peut installer automatiquement ces fichiers utilisateur :

- `~/.config/systemd/user/medalert.service`
- `~/.config/systemd/user/medalert.timer`

Le minuteur execute `medAlert --check` chaque jour a l'heure configuree avec `Persistent=true`.

Le depot contient egalement des fichiers d'exemple dans [systemd/medalert.service](systemd/medalert.service) et [systemd/medalert.timer](systemd/medalert.timer).