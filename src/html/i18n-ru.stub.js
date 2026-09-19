// GerdaLRS Russian Web UI dictionary (NOT wired into build_html.py yet).
// Keys are stock ExpressLRS English UI strings. Values are intended Russian copy.
// Next step: load this map after DOMContentLoaded and replace text nodes /
// labels marked with data-i18n. Keep templates English to save flash.

window.GERDA_I18N_RU = {
  "Welcome to your ExpressLRS System": "GerdaLRS — веб-интерфейс",
  "Runtime Options": "Параметры",
  "Options": "Опции",
  "WiFi": "Wi‑Fi",
  "Update": "Обновление",
  "Model": "Модель",
  "Buttons": "Кнопки",
  "Binding Phrase": "Фраза привязки",
  "Regulatory domain": "Регуляторный домен",
  "Firmware Rev. ": "Ревизия прошивки ",
  "Access Point starting": "Запуск точки доступа"
};

// Planned domain labels (Web UI control not implemented):
window.GERDA_DOMAIN_LABELS_RU = {
  ISM_2400: "ISM 2.4 ГГц (2400–2480)",
  CUSTOM_2640: "CUSTOM_2640 (~2640 МГц, не ISM — ваша ответственность)"
};
