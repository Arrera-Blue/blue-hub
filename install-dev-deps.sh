#!/usr/bin/env bash
# ==============================================================================
# Blue Hub - Arrera Blue OS GUI
# Script d'installation des dépendances de développement (GTK C++ / gtkmm)
# ==============================================================================

set -euo pipefail

# Couleurs Arrera
CYAN='\033[0;36m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
RESET='\033[0m'
BOLD='\033[1m'

echo -e "${CYAN}${BOLD}==========================================================${RESET}"
echo -e "${CYAN}${BOLD}  Blue Hub (abpm GUI) - Installation des dépendances Dev  ${RESET}"
echo -e "${CYAN}${BOLD}==========================================================${RESET}"

# Vérifier si l'utilisateur a les droits root ou sudo
if [ "$EUID" -ne 0 ]; then
    if ! command -v sudo &>/dev/null; then
        echo -e "${RED}[!] Erreur: Ce script nécessite des droits administrateur (sudo ou root).${RESET}" >&2
        exit 1
    fi
    SUDO="sudo"
else
    SUDO=""
fi

# Vérifier la présence de DNF
if ! command -v dnf &>/dev/null; then
    echo -e "${RED}[!] Erreur: Gestionnaire de paquets 'dnf' introuvable.${RESET}" >&2
    exit 1
fi

# 1. Activation du dépôt Copr Arrera Blue si non présent
COPR_REPO="arrera-software/arrera-blue"
echo -e "${YELLOW}[*] Vérification du dépôt Copr '${COPR_REPO}'...${RESET}"

if ! dnf repolist 2>/dev/null | grep -qi "arrera-blue"; then
    echo -e "${YELLOW}[*] Activation du dépôt Copr ${COPR_REPO}...${RESET}"
    $SUDO dnf copr enable -y "$COPR_REPO"
    echo -e "${GREEN}[✓] Dépôt Copr activé avec succès.${RESET}"
else
    echo -e "${GREEN}[✓] Le dépôt Copr est déjà activé.${RESET}"
fi

# 2. Liste des paquets requis
DEV_PACKAGES=(
    # Compilateur et outils de build C++
    gcc-c++
    cmake
    make
    ninja-build
    pkgconf-pkg-config

    # Bibliothèque GTK4 pour C++ (gtkmm 4)
    gtkmm4.0-devel
    gtk4-devel

    # Bibliothèque Adwaita (composants modernes et design GNOME/Arrera)
    libadwaita-devel

    # Gestion des métadonnées logicielles et AppStream (captures, descriptions, catégories)
    appstream-devel

    # Outils pour la gestion des paquets et fichiers
    libarchive-devel
    flatpak-devel

    # Outils de packaging RPM pour Arrera Blue
    rpm-build
    rpmdevtools
    copr-cli

    # CLI abpm depuis le dépôt Copr
    abpm
)

echo ""
echo -e "${YELLOW}[*] Paquets à installer ou mettre à jour :${RESET}"
for pkg in "${DEV_PACKAGES[@]}"; do
    echo -e "  - ${CYAN}${pkg}${RESET}"
done

echo ""
echo -e "${YELLOW}[*] Lancement de l'installation via dnf...${RESET}"
$SUDO dnf install -y "${DEV_PACKAGES[@]}"

echo ""
echo -e "${GREEN}${BOLD}==========================================================${RESET}"
echo -e "${GREEN}${BOLD}[✓] Toutes les dépendances ont été installées avec succès !${RESET}"
echo -e "${GREEN}${BOLD}==========================================================${RESET}"

# Vérifications rapides de version
echo -e "${YELLOW}[*] Vérification de l'environnement :${RESET}"
if pkg-config --exists gtkmm-4.0; then
    GTKMM_VER=$(pkg-config --modversion gtkmm-4.0)
    echo -e " • gtkmm 4 version   : ${CYAN}${GTKMM_VER}${RESET}"
fi

if command -v abpm &>/dev/null; then
    ABPM_VER=$(abpm --version 2>&1 | head -n 1 || true)
    echo -e " • abpm CLI          : ${CYAN}${ABPM_VER}${RESET}"
fi

echo ""
echo -e "${GREEN}[*] L'environnement est prêt pour développer Blue Hub en C++ avec GTK4 / gtkmm.${RESET}"
