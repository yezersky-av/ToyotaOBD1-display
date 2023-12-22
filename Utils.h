String fillWidth(int number, int width = 5, char fillChar = '0') {
  String result = String(number);

  // Вычисление количества символов, которые нужно добавить
  int charsToAdd = (width > result.length()) ? (width - result.length()) : 0;

  // Добавление символов к строке
  for (int i = 0; i < charsToAdd; i++) {
    result = fillChar + result;
  }

  return result;
}

String fillWidth(float number, int width = 5, char fillChar = '0') {
  String result = String(number);

  // Вычисление количества символов, которые нужно добавить
  int charsToAdd = (width > result.length()) ? (width - result.length()) : 0;

  // Добавление символов к строке
  for (int i = 0; i < charsToAdd; i++) {
    result = fillChar + result;
  }

  return result;
}

String fillWidth(String result, int width = 5, char fillChar = ' ') {
  // Вычисление количества символов, которые нужно добавить
  int charsToAdd = (width > result.length()) ? (width - result.length()) : 0;

  // Добавление символов к строке
  for (int i = 0; i < charsToAdd; i++) {
    result = fillChar + result;
  }

  return result;
}


void splitString(const String& input, const String& delimiter, String* output, int& count) {
  // Инициализация переменных
  output[0] = "";
  count = 0;
  int currentIndex = 0;
  int substringStart = 0;

  // Проверка на пустую строку
  if (input.length() == 0) {
    return;
  }

  // Проверка наличия разделителя
  int delimiterLength = delimiter.length();
  int pos = input.indexOf(delimiter);

  if (pos == -1) {
    // Если разделитель не найден, весь вход будет одной подстрокой
    output[currentIndex] = input;
    count = 1;
    return;
  }

  // Перебор символов во входной строке
  while (pos != -1) {
    // Завершаем текущую подстроку и переходим к следующей
    output[currentIndex++] = input.substring(substringStart, pos);
    substringStart = pos + delimiterLength;

    // Поиск следующего разделителя
    pos = input.indexOf(delimiter, substringStart);
  }

  // Добавляем последнюю подстроку (если она не заканчивается разделителем)
  output[currentIndex] = input.substring(substringStart);
  count = currentIndex + 1;
}