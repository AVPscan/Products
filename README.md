# 🛒 Smart Shopping & Price Analytics / Умные покупки и аналитика цен
![Linux Build status](https://github.com)
![Windows Build status](https://github.com)

![Linux Build status](https://github.com)
![Windows Build status](https://github.com)

**Author:** Алексей Васильевич Поздняков (Alexey V. Pozdnyakov)  
**Email:** avp70ru@mail.ru  
**License:** GPLv3

---

## 🌍 Global Concept / Глобальная концепция
A cross-platform tool for smart shopping. The interface uses **emojis** instead of words, making it accessible to anyone in the world, regardless of their language.
Кроссплатформенная утилита для умных покупок. Интерфейс использует **эмодзи** вместо слов, что делает его понятным любому человеку в мире.

## ⌨️ Control & Features / Управление и возможности
*   **Input / Ввод**: 
    *   `Alphabet` — Product names / Имя товара.
    *   `Digits` — Prices / Цена.
    *   `Left + Digit` — Set quantity / Ввод количества (стрелка Влево + цифра).
*   **Fast Add / Быстрый ввод**: 
    *   Real-time suggestions. If there's one match and price > 0, press `Enter` to add.
    *   Подсказки на лету. Если совпадение одно и цена введена — `Enter` добавит товар.
*   **Analytics / Аналитика**: 
    *   `Up` / `Down` keys to browse historical weighted average prices.
    *   Клавиши `Вверх` / `Вниз` для просмотра средневзвешенных цен из прошлых покупок.
*   **Exit & Sync / Выход и синхронизация**: 
    *   `Esc` — Calculate total and save files.
    *   If `send.txt` exists, `reports.txt` is automatically sent to mail.
    *   `Esc` — Расчет суммы и сохранение. Если есть `send.txt`, отчет улетает на почту.

## 📂 File Structure / Структура файлов
*   `products.txt` — Current shopping list.
*   `analitics.txt` — Cumulative price history.
*   `reports.txt` — Final report (sent via email).

---
*Created in 2026. Non-blocking UI for maximum efficiency.*
## 🛠 Build & Tech / Сборка и Оптимизация
Проект кроссплатформенный и автоматически адаптируется под систему (Linux/Windows) через `makefile`.

*   **Extreme Optimization**: Бинарный файл весит менее **27 КБ**.
*   **Performance**: Благодаря сверхмалому размеру, исполняемый код целиком помещается в **L1-кэш процессора**, что обеспечивает максимально возможную скорость работы и мгновенный отклик интерфейса.
*   **Static Analysis**: Чистый C11 без тяжелых зависимостей.

```bash
make         # Сборка оптимизированной (tiny) версии
./products   # Запуск
products.exe #
