# daemon_shm
Projet de licence 3 visant à développer un programme démon et son programme client afin d'exécuter des commandes demandés par le client sur les threads du démon.

## Fonctionnement
![alt text](https://i.imgur.com/6bhG5GK.png)

Ces deux programmes sont créé pour fonctionner sur un système Linux.

Le démon accueille les messages des clients à l'aide d'un tube qu'il créé au début de son exécution puis renvoie les informations de connexion au segment de mémoire partagé (SHM) du thread alloué dans le tube du client pour qu'il puisse exécuter ses commandes dessus.

## Configuration
```
MIN_THREAD=2
MAX_THREAD=4
MAX_CONNECT_PER_THREAD=0
SHM_SIZE=1024
```
Ceci est le contenu du fichier demon.conf disponible dans le dossier démon, voici sa sémentique :

- MIN_THREAD : le nombre minimum de thread lancé.
- MAX_THREAD : le nombre de thread maximum lancé.
- MAX_CONNECT_PER_THREAD : le nombre de connections par thread (0 = pas de limite).
- SHM_SIZE : la taille en octets de la mémoire partagé entre le client et le thread.

## Mise en place
Il est requis de lancer le démon en premier, on peut exécuter ce programme à l'aide de l'option "--debug" qui permet d'annuler sa transformation en démon et de monitorer son fonctionnement.

Une fois le démon exécuté, on peut lancer un nombre de terminal souhaité afin de lancer le programme client sur chaque terminal.
Toute les connections sont établis quand le message "client>" apparait sur un terminal client, on peut désormais saisir nos commandes.

## Exemples

Fonctionnement normal avec 2 clients

![alt text](https://i.imgur.com/v6enQBS.png)

Usure d'un thread et test du minimum de thread

![alt text](https://i.imgur.com/cGSgVTu.png)

Limite de connexions au démon

![alt text](https://i.imgur.com/eFKeRk9.png)




## Valgrind

Programme démon

![alt text](https://i.imgur.com/kDKjYZZ.png)

Programme client

![alt text](https://i.imgur.com/P2s2RtG.png)
