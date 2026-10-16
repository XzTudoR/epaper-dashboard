# Google Apps Script Backend Setup

Acest script servește drept backend serverless pentru ecranul E-Paper, agregând datele din:
- **Google Calendar** (orar cursuri și evenimente)
- **Google Contacts** (zile de naștere)
- **Google Tasks** (to-do list activ)
- **Open-Meteo API** (prognoză meteo fără cheie API necesară)

---

## Ghid Pas cu Pas pentru Deploiere

### 1. Crearea Proiectului
1. Deschide [Google Apps Script](https://script.google.com/home).
2. Apasă pe butonul **New project** (Proiect nou).
3. Redenumește proiectul în `epaper-backend`.

### 2. Copierea Codului
1. Înlocuiește conținutul fișierului `Code.gs` cu codul din [`Code.gs`](./Code.gs).
2. Configurează coordonatele geografice (`LATITUDE`, `LONGITUDE`) și fusul orar (`TIMEZONE`) în obiectul `CONFIG` din partea de sus a fișierului, dacă este cazul.

### 3. Activarea Serviciului Google Tasks
Pentru a citi task-urile, trebuie activat serviciul **Tasks**:
1. În meniul din stânga, apasă pe semnul `+` de lângă **Services** (Servicii).
2. Caută și selectează **Tasks API**.
3. Păstrează versiunea `v1` și identificatorul `Tasks`.
4. Apasă **Add** (Adăugare).

### 4. Deploierea ca Web App
1. În colțul din dreapta-sus, apasă pe **Deploy** -> **New deployment** (Implementare nouă).
2. Apasă pe rotița de configurare (iconița de setări) de lângă *Select type* și alege **Web app**.
3. Completează câmpurile:
   - **Description**: `E-Paper Dashboard API v1`
   - **Execute as**: `Me` (adresa ta de Google)
   - **Who has access**: `Anyone` (Oricine — necesar pentru ca ESP32 să poată citi JSON-ul fără autentificare OAuth complexă)
4. Apasă **Deploy**.
5. În fereastra pop-up, acordă permisiunile solicitate (*Review Permissions* -> alege contul tău -> *Advanced* -> *Go to epaper-backend (unsafe)* -> *Allow*).
6. Copiază **Web app URL** generat (are formatul `https://script.google.com/macros/s/AKfycb.../exec`).

### 5. Integrare cu Firmware-ul ESP32
Lipește URL-ul obținut în fișierul `firmware/epaper_dashboard/config.h`:
```cpp
const char* const API_URL = "https://script.google.com/macros/s/AKfycb.../exec";
```

---

## Testare Rapidă

Poți testa endpoint-ul direct în browser sau rulând comanda `curl`:
```bash
curl -L "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"
```

Răspunsul așteptat este un obiect JSON de forma:
```json
{
  "ziua": "VINERI",
  "data": "17 octombrie",
  "vremea": "18°C",
  "meteo_icon": 0,
  "orar": [
    "10:00 Curs Inteligenta Artificiala (Amf. 3)",
    "14:00 Laborator Sisteme Embedate (Lab B2)"
  ],
  "tasks": [
    "Finalizare tema Arduino",
    "Trimitere raport laborator"
  ],
  "evenimente": [
    "bday Tudor - 17 oct (Azi!)"
  ]
}
```
