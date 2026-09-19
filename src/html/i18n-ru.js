// GerdaLRS Russian string table. Included into scan.js / hardware.js at build time.
// Keys are stable English UI ids (data-i18n). Visible copy is Russian.

window.GERDA_I18N_RU = {
  "Model": "Модель",
  "Options": "Опции",
  "WiFi": "Wi‑Fi",
  "Update": "Обновление",
  "Buttons": "Кнопки",
  "Runtime Options": "Параметры",
  "Firmware Update": "Обновление прошивки",
  "Button Actions": "Действия кнопок",
  "PWM Output": "Выходы PWM",
  "Binding Phrase": "Фраза привязки",
  "Serial Protocol": "Последовательный протокол",
  "Model Match": "Совпадение модели",
  "Force telemetry off": "Принудительно выключить телеметрию",
  "Save": "Сохранить",
  "Confirm": "Подтвердить",
  "Regulatory domain": "Регуляторный домен (900 МГц)",
  "2.4 / 2640 band": "Диапазон 2.4 / 2640",
  "ISM 2.4 GHz": "ISM 2.4 ГГц (2400–2480)",
  "CUSTOM_2640": "CUSTOM_2640 (~2640 МГц, не ISM)",
  "Gerda Secure Link": "Защищённый линк Gerda",
  "Flight profile": "Профиль полёта",
  "Profile Balance": "Баланс (как в Lua)",
  "Profile Range": "Дальность (50 Гц, телеметрия 1:16)",
  "Profile Speed": "Скорость (500 Гц, телеметрия 1:64)",
  "Firmware Rev. ": "Ревизия прошивки ",
  "Import/Export": "Импорт/экспорт",
  "Currently in Access Point mode": "Режим точки доступа",
  "Reset runtime options to defaults": "Сбросить параметры к значениям прошивки",
  "WiFi auto-on interval": "Интервал авто-включения Wi‑Fi, сек (пусто = выкл)",
  "TLM report interval": "Интервал телеметрии (мс)",
  "Fan runtime": "Время работы вентилятора (с)",
  "Use as AirPort Serial device": "Режим AirPort (прозрачный UART)",
  "AirPort UART baud": "Скорость UART AirPort",
  "UART baud": "Скорость UART",
  "Lock on first connection": "Фиксировать после первого соединения",
  "Permanently arm DJI": "Постоянно держать DJI air unit в ARM",
  "Save model configuration file": "Сохранить файл конфигурации моделей"
};

window.GERDA_I18N_RU_ATTR = {
  "Binding Phrase": "Фраза привязки"
};

function gerdaApplyI18n() {
  document.documentElement.lang = 'ru';
  const dict = window.GERDA_I18N_RU || {};
  document.querySelectorAll('[data-i18n]').forEach((el) => {
    const key = el.getAttribute('data-i18n');
    if (dict[key]) {
      el.textContent = dict[key];
    }
  });
  document.querySelectorAll('[data-i18n-placeholder]').forEach((el) => {
    const key = el.getAttribute('data-i18n-placeholder');
    if (window.GERDA_I18N_RU_ATTR && window.GERDA_I18N_RU_ATTR[key]) {
      el.setAttribute('placeholder', window.GERDA_I18N_RU_ATTR[key]);
    }
  });
}
