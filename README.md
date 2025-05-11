# 🌱 TerraLink – Inteligentny system nadzoru wzrostu roślin

**TerraLink** to energooszczędny system do monitorowania i zarządzania warunkami wzrostu roślin, oparty na technologii mikrokontrolerów STM32 oraz komunikacji LoRa. Projekt składa się z bezprzewodowych czujników glebowych i centralnego sterownika zbierającego dane i prezentującego je w aplikacji webowej lub mobilnej.

---

## 🔧 Architektura systemu

### 🌱 Doniczka TerraNode (czujnik glebowy)
- Wbudowany mikrokontroler STM32L031K6T6 (Ultra Low-Power)
- Komunikacja LoRa (moduł SX1278 RA-02, 433 MHz)
- Pomiary środowiskowe:
  - Wilgotność gleby (czujnik pojemnościowy)
  - Temperatura i wilgotność powietrza (SHT31)
  - Natężenie światła (BH1750)
  - pH gleby
- Zasilanie:
  - Akumulator Li-Ion
  - Ładowanie przez USB-C oraz ładowanie solarne
- Tryb oszczędzania energii:
  - Mikrokontroler przez większość czasu jest Stop2
  - Wybudzanie przez przerwania jedynie na czas pomiarów

### 📡 Sterownik TerraLink (centralny hub)
- Mikrokontroler STM32L476RG
- Obsługa wielu doniczek
- Komunikacja dwukierunkowa z doniczkami (LoRa)
- Transmisja danych do interfejsu (Wi-Fi)
- Konfiguracja i identyfikacja urządzeń
- Możliwość zdalnego wybudzania doniczek

---

## 🖥️ Interfejs użytkownika

- Aplikacja webowa
- Aplikacja mobilna z powiadomieniami
- Prezentacja danych w czasie rzeczywistym
- Podgląd statusu każdej doniczki (wilgotność, temperatura, światło, bateria)
- Możliwość konfiguracji i zarządzania urządzeniami

---

## ⚙️ Technologie i narzędzia

- STM32CubeIDE, HAL
- Embedded C
- LoRa (SX1278)
- Praca na systemach zasilania bateryjnego (low-power)
- RTC, ADC, EXTI, I2C, SPI
- Pamięć Flash

---

## 🔋 Zasilanie i energooszczędność

- Doniczki pracują w trybie deep sleep i wybudzają się okresowo lub na żądanie
- Przewidywana żywotność baterii: wiele miesięcy
- Zużycie energii ograniczone przez użycie komponentów niskopoborowych i przemyślaną logikę działania
