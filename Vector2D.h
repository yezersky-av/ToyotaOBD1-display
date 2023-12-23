class Vector2D {
public:
  float x;
  float y;

  // Конструктор класса
  Vector2D(float xVal = 0.0, float yVal = 0.0) : x(xVal), y(yVal) {}

  // Конструктор копирования
  Vector2D(const Vector2D& other) : x(other.x), y(other.y) {}

  // Перегрузка оператора присваивания
  Vector2D& operator=(const Vector2D& other) {
    if (this != &other) { // Проверка на самоприсваивание
      x = other.x;
      y = other.y;
    }
    return *this;
  }

  // Метод для вращения вектора на заданный угол
  void rotate(float angle) {
    float newX = x * cos(angle) - y * sin(angle);
    float newY = x * sin(angle) + y * cos(angle);

    x = newX;
    y = newY;
  }

  // Метод для расчёта длины вектора
  float length() const {
    return sqrt(x * x + y * y);
  }

 // Метод для нормализации вектора (приведения к единичной длине)
  Vector2D normalize() const {
    float len = length();
    if (len > 0) {
      return Vector2D(x / len, y / len);
    } else {
      return *this;  // Вектор уже нулевой длины, возвращаем копию текущего вектора
    }
  }

  // Перегрузка оператора сложения
  Vector2D operator+(const Vector2D& other) const {
    return Vector2D(x + other.x, y + other.y);
  }

  // Перегрузка оператора вычитания
  Vector2D operator-(const Vector2D& other) const {
    return Vector2D(x - other.x, y - other.y);
  }
};

class LineSegment {
public:
  Vector2D start;  // Начальная точка
  Vector2D end;    // Конечная точка

// Конструктор класса
  LineSegment(const Vector2D& startPoint = Vector2D(), const Vector2D& endPoint = Vector2D()) : start(startPoint), end(endPoint) {}

  // Метод для вычисления длины отрезка
  float length() const {
    return (end - start).length();
  }

  // Метод для определения точки пересечения между отрезком и вектором
  Vector2D intersect(const Vector2D& point, const Vector2D& direction) const {
    // Уравнение отрезка: P = start + t * (end - start), где 0 <= t <= 1
    // Уравнение вектора: Q = point + s * direction

    // Вычисляем детерминант для определения коллинеарности
    float det = (direction.y * (end.x - start.x)) - (direction.x * (end.y - start.y));

    // Проверка на коллинеарность
    if (det == 0) {
      // Отрезок и вектор коллинеарны или параллельны, возвращаем нулевой вектор
      return Vector2D(0, 0);
    }

    // Вычисляем параметры t и s
    float t = ((point.x - start.x) * (end.y - start.y) - (point.y - start.y) * (end.x - start.x)) / det;
    float s = ((point.x - start.x) * direction.y - (point.y - start.y) * direction.x) / det;

    // Проверка условий
    if (t >= 0 && t <= 1 && s >= 0 && s <= 1) {
      // Найдено пересечение, возвращаем точку пересечения
      return Vector2D(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y));
    } else {
      // Пересечение не найдено, возвращаем нулевой вектор
      return Vector2D(0, 0);
    }
  }

  Vector2D intersect(const LineSegment& other) const {
  // Уравнение первого отрезка: P = start + t * (end - start), где 0 <= t <= 1
  // Уравнение второго отрезка: Q = other.start + s * (other.end - other.start), где 0 <= s <= 1

  // Вычисляем детерминант для определения коллинеарности
  float det = (other.end.y - other.start.y) * (end.x - start.x) - (other.end.x - other.start.x) * (end.y - start.y);

  // Проверка на коллинеарность
  if (det == 0) {
    // Отрезки коллинеарны или параллельны, возвращаем нулевой вектор
    return Vector2D(0, 0);
  }

  // Вычисляем параметры t и s
  float t = ((other.start.x - start.x) * (other.end.y - other.start.y) - (other.start.y - start.y) * (other.end.x - other.start.x)) / det;
  float s = ((other.start.x - start.x) * (end.y - start.y) - (other.start.y - start.y) * (end.x - start.x)) / det;

  // Проверка условий
  if (t >= 0 && t <= 1 && s >= 0 && s <= 1) {
    // Найдено пересечение, возвращаем точку пересечения
    return Vector2D(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y));
  } else {
    // Пересечение не найдено, возвращаем нулевой вектор
    return Vector2D(0, 0);
  }
}
};

class Shape {
public:
  static const int MAX_VECTORS = 20;
  Vector2D vectors[MAX_VECTORS];
  LineSegment segments[MAX_VECTORS];
  int vectorCount;

  // Конструктор класса
  Shape(){}

  // Метод для добавления вектора в форму
  void addVector(const Vector2D& vector) {
    if (vectorCount < MAX_VECTORS) {
      vectors[vectorCount] = vector;
      vectorCount++;
    }
  }

  // Метод для очистки данных формы
  void clear() {
    vectorCount = 0;
  }

  // Метод для создания отрезков из векторов
  void createSegments() {
    for (int i = 0; i < vectorCount; i++) {
      int next = i + 1 >= vectorCount ? 0 : i + 1;
      segments[i] = LineSegment(vectors[i], vectors[next]);
    }
  }

  // Метод для определения, находится ли точка внутри формы
  bool isPointInside(const Vector2D& point) const {
    if (vectorCount < 3) {
      // Форма не может быть замкнутой с меньше чем тремя векторами
      return false;
    }

    int intersections = 0;

    // Перебираем все отрезки формы
    for (int i = 0; i < vectorCount - 1; ++i) {
      // Проверяем, пересекается ли луч с отрезком
      Vector2D intersection = segments[i].intersect(point, Vector2D(1.0, 0.0));
      if (intersection.x > 0) {
        intersections++;
      }
    }

    // Форма замкнута, добавляем проверку для последнего отрезка
    Vector2D intersection = segments[vectorCount - 2].intersect(point, Vector2D(1.0, 0.0));
    if (intersection.x > 0) {
      intersections++;
    }

    // Если количество пересечений нечетное, точка внутри формы
    return intersections % 2 != 0;
  }

  // Метод для нахождения точки пересечения вектора с формой
  Vector2D findIntersection(const Vector2D& start, const Vector2D& direction) const {
    if (vectorCount < 3) {
      // Форма должна содержать хотя бы три вектора
      return Vector2D(0, 0);
    }

    // Создаем временный отрезок для поиска пересечения
    LineSegment tempSegment(start, start + direction);

    // Перебираем все отрезки формы
    for (int i = 0; i < vectorCount; i++) {
      // Проверяем, пересекается ли временный отрезок с текущим отрезком формы
      Vector2D intersection = segments[i].intersect(tempSegment);

      if(intersection.length() > 0){
        return intersection;
      }
    }

    // Если не найдено пересечение, возвращаем нулевой вектор
    return Vector2D(0, 0);
  }

    // Метод для нахождения точки пересечения вектора с формой
  Vector2D findIntersection(const LineSegment& line) const {
    if (vectorCount < 3) {
      // Форма должна содержать хотя бы три вектора
      return Vector2D(0, 0);
    }

    // Перебираем все отрезки формы
    for (int i = 0; i < vectorCount; i++) {
      // Проверяем, пересекается ли временный отрезок с текущим отрезком формы
      Vector2D intersection = segments[i].intersect(line);

      if(intersection.length() > 0){
        return intersection;
      }
    }

    // Если не найдено пересечение, возвращаем нулевой вектор
    return Vector2D(0, 0);
  }
};
