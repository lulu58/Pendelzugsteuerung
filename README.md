# Pendelzugsteuerung mit Arduino und L298 H-Bridge Motorsteuerung
- Pinbelegung und verwendete Timer gelten für Arduino UNO und sind gegebenenfalls anzupassen 
- Geschwindigkeit und Richtung über Pulsweitenmodulation und H-Brücke
- vor den Enden der Fahrstrecke sind 2 Sensoren (Reedkontakte, Lichtschranken, ...) angeordnet, ab denen der Bremsvorgang eingeleitet wird
- Wartezeiten links und rechts getrennt festlegbar
- langsames Anfahren in mehreren Fahrstufen
- Abbremsen in mehreren Fahrstufen
- Minimal- und Maximalgeschwindigkeit, Anfahr- und Bremszeit sowie Anzahl der Fahrstufen als Konstanten im Code
- Timeout-Überwachung de Fahrstrecke für zusätzliche Sicherheit
- hohe PWM-Pulsfrequenz von ca. 30 kHz vermeidet zusätzliche Motorgeräusche
- Notaus über weitere parallel geschaltete Schaltkontakte möglich (am Bedienpult, vor Prellböcken)

Angelehnt an (c) Michael Schoeffler 2016 (http://www.mschoeffler.de), und (c) 2017 Ralf Junius (http://z-freunde-international.de/index.php/ralfs-pendelzugsteuerung.html)

Bisher ist nur ein Testbetrieb mit einer LGB-Lok auf dem Teppich durchgeführt worden, das Feintuning erfolgt an der noch zu bauenden Strecke.
