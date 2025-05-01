# ajedrezUPV

**ajedrezUPV** es un proyecto de inteligencia artificial aplicada al ajedrez, desarrollado en el marco de la Universitat Politècnica de València (UPV).  
Este motor de ajedrez tiene como objetivo fomentar el estudio de algoritmos de decisión, heurísticas de evaluación y técnicas de programación de motores de juego.

## Características

- Implementación de motor de ajedrez propio
- Algoritmos de búsqueda y evaluación heurística
- Código modular y extensible
- Enfocado en la investigación educativa y la práctica universitaria

## Requisitos

- Lenguaje: C++23
- Usamos Disservin (https://disservin.github.io/chess-library/), que nos implementa las reglas, entre otras cosas muy útiles.
- Usamos Json C++ (https://github.com/nlohmann/json), que nos ayuda a leer archivos jsonl de la database de lichess. 

## Instalación
Podéis instalarlo así usando git:

git clone https://github.com/LLPB-pixel/ajedrezUPV.git
cd ajedrezUPV

O manualmente descargando lo que hay en implementaciones.
En cualquier caso es necesario compilar con:
g++ [versiondelbot] NodeMove.cpp GeneralEvaluator.cpp OpeningEvaluator.cpp EndgameEvaluator.cpp 
Saldrá una aplicación de consola. Sólo acepta jugadas en notación UCI. ( https://es.wikipedia.org/wiki/Interfaz_Universal_de_Ajedrez )

Cuando desarrolle la IA habrá que compilar de forma distinnta, lógicamente.
Para cualquier duda, bug, enviadme un correo a perezllorenc@gmail.com


## Licencia
Este proyecto está licenciado bajo la [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).
![Licencia](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)







