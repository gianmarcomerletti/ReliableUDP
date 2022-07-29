# ReliableUDP
Trasferimento file con protocollo UDP e con implementazione Go Back N

Il progetto consiste in una semplice applicazione di tipo `client-server` di tipo `concorrente`, che permette al client di scaricare file dal server attraverso un comando `get`, caricare dei propri files sul server stesso tramite un comando `put` e ottenere una lista di tutti i files presenti nel server con il comando `list`.
<br><br>Questa applicazione per il trasferimento di file utilizza come protocollo di trasporto `UDP`, ovvero un protocollo che risulta essere di base non affidabile essendo senza connessione e privo di un meccanismo di riscontro.
<br><br>Lo scopo del progetto è stato quello di utilizzare UDP rendendolo un protocollo il più affidabile possibile, attraverso un meccanismo di affidabilità basato sul protocollo `Go-Back N`, in modo da garantire una corretta trasmissione di file e messaggi tra client e server.
