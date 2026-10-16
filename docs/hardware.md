# Hardware & Wiring Guide

Acest document descrie componentele hardware, conexiunile electrice (pinout) și optimizările de consum de energie pentru proiectul E-Paper Smart Dashboard.

---

## 🔌 Specificații Componente

1. **Microcontroller**: ESP32 (ESP32-C3 / ESP32-S3 / standard ESP32 DevKit)
   - Conectivitate: Wi-Fi 802.11 b/g/n (2.4 GHz)
   - Funcționalitate Deep Sleep pentru autonomie pe baterie
2. **Display E-Paper**:
   - Model: **Good Display GDEY042T81** (sau panouri Waveshare 4.2" compatibile)
   - Rezoluție: **400 x 300 pixeli**
   - Culori: Alb / Negru (1-bit monocrom)
   - Tehnologie: E-Ink bistabil (menține imaginea afișată fără alimentare)
   - Driver GxEPD2: `GxEPD2_420_GDEY042T81`

---

## 📌 Tabel de Conexiuni (Pinout SPI)

| Pin E-Paper | Semnal | Pin ESP32 (Implicit) | Notă |
|:---:|:---:|:---:|:---|
| **VCC** | 3.3V | **3V3** | Alimentare logică 3.3V (NU conectați la 5V!) |
| **GND** | Masă | **GND** | Referință electrică comună |
| **DIN / MOSI** | SPI Data | **GPIO 7** | Master Out Slave In |
| **CLK / SCK** | SPI Clock | **GPIO 6** | Serial Clock |
| **CS** | Chip Select | **GPIO 5** | Activ pe LOW |
| **DC** | Data / Command | **GPIO 4** | LOW = Comandă, HIGH = Date |
| **RST** | Reset | **GPIO 2** | Hardware Reset (activ pe LOW) |
| **BUSY** | Stare Panou | **GPIO 15** | HIGH = Ocupat (refresh în curs) |

> [!NOTE]
> Dacă folosiți o altă variantă de placă ESP32 (de exemplu un ESP32 NodeMCU clasic cu pinii SPI impliciți 18/23), puteți schimba cu ușurință pinii în fișierul `config.h`:
> ```cpp
> constexpr int PIN_EPD_CS   = 5;
> constexpr int PIN_EPD_DC   = 4;
> constexpr int PIN_EPD_RST  = 2;
> constexpr int PIN_EPD_BUSY = 15;
> constexpr int PIN_SPI_SCK  = 6;
> constexpr int PIN_SPI_MOSI = 7;
> ```

---

## ⚡ Optimizare Consum și Baterie

Proiectul este proiectat pentru a funcționa săptămâni sau luni pe un acumulator LiPo (3.7V / 18650 sau similar):

1. **Ciclul de Viață (Lifecycle)**:
   - ESP32 se trezește din Deep Sleep (`esp_sleep_enable_timer_wakeup`).
   - Pornește SPI și inițializează ecranul.
   - Conectează Wi-Fi și sincronizează ceasul via NTP.
   - Apelează Google Apps Script, primește JSON-ul și desenează pe ecran.
   - Trece panoul E-Paper în modul `hibernate()`.
   - Oprește modemul Wi-Fi (`WiFi.disconnect(true); WiFi.mode(WIFI_OFF);`).
   - Intră în Deep Sleep timp de 3 ore (`TIME_TO_SLEEP_SEC`).
2. **Consum Estimat**:
   - **Deep Sleep**: ~10–25 µA (în funcție de placa de dezvoltare și regulatorul LDO).
   - **Trezire activă (Wi-Fi + Refresh)**: ~80–120 mA timp de aproximativ 5–8 secunde.
   - **Consum ecran când e deconectat**: 0 mA (imaginea rămâne permanent pe ecran).
