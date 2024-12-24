#include <Servo.h>
#include <math.h>

// Pines Joystick
const int VRxPin = A0; // Eje X
const int VRyPin = A1; // Eje Y
const int SWPin  = 2;  // Botón pulsador

// Pines Servos
const int servoPin        = 9;  // Servo principal (horizontal)
const int servoAltura     = 10; // Servo vertical 1
const int servoAltura2    = 11; // Servo vertical 2
const int servoGarra      = 12; // Servo para la garra

// Creación de objetos de tipo Servo
Servo myServo;         // Servo horizontal
Servo alturaServo;     // Servo vertical 1
Servo altura2Servo;    // Servo vertical 2
Servo garraServo;      // Servo de la garra

// Variables para el estado de la garra
bool garraAbierta = false;
int garraPositionOpen    = 180; // Ángulo "abierto" de la garra (ajusta según tus necesidades)
int garraPositionClosed  = 120; // Ángulo "cerrado" de la garra (ajusta según tus necesidades)

// Variables de estado y posición de servos
unsigned long lastStateChangeTime = 0;
unsigned long currentTime;
String currentState  = "Quieto";
String previousState = "";

// Posiciones iniciales de los servos horizontal y vertical
int servoPosition        = 90;  // Inicialmente centrado
int servoAlturaPosition  = 30;  // Limitado a 0-65 grados
int servoAltura2Position = 0;   // Inicialmente en 0

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  // Joystick: el botón SWPin con resistencia interna de pull-up
  pinMode(SWPin, INPUT_PULLUP);

  // Adjuntar servos a sus pines
  myServo.attach(servoPin);
  alturaServo.attach(servoAltura);
  altura2Servo.attach(servoAltura2);
  garraServo.attach(servoGarra);

  // Inicializar posiciones de servos
  myServo.write(servoPosition);           // Centro horizontal
  alturaServo.write(servoAlturaPosition); // Centro vertical 1
  altura2Servo.write(servoAltura2Position); // Inicialmente en 0
  garraServo.write(garraPositionClosed);  // Inicia con la garra cerrada
}

// ----------------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------------
void loop() {
  int xValue = analogRead(VRxPin);
  int yValue = analogRead(VRyPin);

  // Determina el nuevo estado según posición del joystick
  String newState = determineState(xValue, yValue);

  // Si el estado ha cambiado, registrar tiempo y mostrar en Serial
  if (newState != currentState) {
    currentTime = millis();
    if (currentState != "") {
      unsigned long elapsedTime = currentTime - lastStateChangeTime;
      Serial.print("Estado anterior: ");
      Serial.print(currentState);
      Serial.print(" | Tiempo: ");
      Serial.print(elapsedTime / 1000.0, 2);
      Serial.println(" s");
    }
    currentState = newState;
    lastStateChangeTime = currentTime;
  }

  // Calcular tiempo transcurrido y log en base 2
  unsigned long elapsedInState = millis() - lastStateChangeTime;
  double logValue = calculateLogarithmBase2(elapsedInState / 1000.0);

  // Calcula un factor de intensidad según la distancia del joystick al centro
  double intensityFactor = calculateIntensityFactor(xValue, yValue);

  // Ajustar movimiento del servo en función del log y el factor de intensidad
  double adjustedMovement = logValue * intensityFactor;

  // Mostrar por Serial para depuración
  Serial.print("Estado: ");
  Serial.print(currentState);
  Serial.print(" | Log ajustado: ");
  Serial.print(adjustedMovement, 2);
  Serial.print(" | Factor de intensidad: ");
  Serial.println(intensityFactor, 2);

  // Actualizar ambos servos (horizontal y vertical) según el estado
  updateServos(adjustedMovement, currentState);

  // Manejar la garra (abrir/cerrar) mediante el botón pulsador
  handleGarra();

  // Pequeño retardo para "refrescar" la lectura del joystick
  delay(10);
}

// ----------------------------------------------------------------------------
// FUNCIONES AUXILIARES
// ----------------------------------------------------------------------------

// Determina el estado en base a la posición del joystick
String determineState(int x, int y) {
  const int threshold = 50;
  const int centerX   = 512;
  const int centerY   = 512;

  if (abs(x - centerX) < threshold && abs(y - centerY) < threshold) 
    return "Quieto";
  if (x - centerX > threshold)   return "Derecha";
  if (centerX - x > threshold)   return "Izquierda";
  if (y - centerY > threshold)   return "Atrás";
  if (centerY - y > threshold)   return "Adelante";

  return "Desconocido";
}

// Calcula logaritmo base 2 multiplicado por 2 (con límite mínimo)
double calculateLogarithmBase2(double time) {
  if (time <= 0) return 0;
  double adjustedLog = (log(time) / log(2)) * 2; // Ajustar base y escala
  return adjustedLog > 0 ? adjustedLog : 1;
}

// Calcula la intensidad de movimiento en función de la distancia del joystick al centro
double calculateIntensityFactor(int x, int y) {
  const int centerX    = 512;
  const int centerY    = 512;
  const int maxOffset  = 512; // Máximo desplazamiento posible del joystick
  const int threshold  = 50;  // Pequeño margen muerto

  int offsetX = abs(x - centerX);
  int offsetY = abs(y - centerY);
  double offset = max(offsetX, offsetY);

  // Si la distancia es muy pequeña, evitamos movimientos
  if (offset < threshold) return 1.0;

  double adjustedOffset = offset - threshold;
  double adjustedMax    = maxOffset - threshold;
  double intensityFactor = 1.0 + (3.0 * adjustedOffset / adjustedMax);

  return constrain(intensityFactor, 0.1, 3.0);
}

// Actualiza la posición de ambos servos (horizontal y vertical)
void updateServos(double movement, String state) {
  int movementAmount = (int)movement;

  // Variables objetivo para cada servo
  int horizontalTargetPosition = servoPosition;
  int verticalTargetPosition1  = servoAlturaPosition;
  int verticalTargetPosition2  = servoAltura2Position;

  // Asigna el movimiento según el estado
  if (movementAmount > 0) {
    if (state == "Izquierda") {
      horizontalTargetPosition -= movementAmount;   // Gira hacia la izquierda
    } else if (state == "Derecha") {
      horizontalTargetPosition += movementAmount;   // Gira hacia la derecha
    } else if (state == "Adelante") {
      // Ajusta la altura hacia adelante
      // Primero mueve el servo de altura 1
      verticalTargetPosition1 -= movementAmount;   
      // Si se excede, mueve el servo de altura 2
      if (verticalTargetPosition1 < 0) {
        verticalTargetPosition2 += abs(verticalTargetPosition1);
        verticalTargetPosition1 = 0;
      }
    } else if (state == "Atrás") {
      // Ajusta la altura hacia atrás
      // Primero mueve el servo de altura 1
      verticalTargetPosition1 += movementAmount;
      // Si se excede, mueve el servo de altura 2
      if (verticalTargetPosition1 > 65) {
        verticalTargetPosition2 += (verticalTargetPosition1 - 65);
        verticalTargetPosition1 = 65;
      }
    }

    // Restringir horizontal a [0,180]
    horizontalTargetPosition = constrain(horizontalTargetPosition, 0, 180);

    // Restringir vertical1 a [0,65]
    verticalTargetPosition1 = constrain(verticalTargetPosition1, 0, 65);

    // Restringir vertical2 a [0,65]
    verticalTargetPosition2 = constrain(verticalTargetPosition2, 0, 65);

    // Calcular el total de altura
    int totalAltura = verticalTargetPosition1 + verticalTargetPosition2;

    // Limitar totalAltura a 130 grados
    if (totalAltura > 130) {
      totalAltura = 130;
      // Ajustar vertical2Position si es necesario
      verticalTargetPosition2 = min(65, totalAltura - verticalTargetPosition1);
    }

    // Actualizar los objetivos de los servos
    verticalTargetPosition1 = constrain(verticalTargetPosition1, 0, 65);
    verticalTargetPosition2 = constrain(verticalTargetPosition2, 0, 65);

    // Actualizar los objetivos globales
    servoPosition = horizontalTargetPosition;
    servoAlturaPosition = verticalTargetPosition1;
    servoAltura2Position = verticalTargetPosition2;

    // Mover gradualmente los servos horizontal y vertical
    // Salimos del while si el estado cambia (ya no es el mismo).
    while ((myServo.read() != horizontalTargetPosition) || 
           (alturaServo.read() != verticalTargetPosition1) ||
           (altura2Servo.read() != verticalTargetPosition2)) {
      
      // Leer el joystick y determinar si el estado cambió
      int x = analogRead(VRxPin);
      int y = analogRead(VRyPin);
      String currentCheckState = determineState(x, y);

      // Si cambia el estado (por ejemplo, de 'Izquierda' a 'Quieto'), detén el movimiento
      if (currentCheckState != state) {
        Serial.println("Cambio de estado detectado, deteniendo movimiento.");
        break; 
      }

      // Ajustar el servo horizontal
      if (myServo.read() < horizontalTargetPosition) {
        myServo.write(myServo.read() + 1);
      } else if (myServo.read() > horizontalTargetPosition) {
        myServo.write(myServo.read() - 1);
      }

      // Ajustar el servo vertical 1
      if (alturaServo.read() < verticalTargetPosition1) {
        alturaServo.write(alturaServo.read() + 1);
      } else if (alturaServo.read() > verticalTargetPosition1) {
        alturaServo.write(alturaServo.read() - 1);
      }

      // Ajustar el servo vertical 2
      if (altura2Servo.read() < verticalTargetPosition2) {
        altura2Servo.write(altura2Servo.read() + 1);
      } else if (altura2Servo.read() > verticalTargetPosition2) {
        altura2Servo.write(altura2Servo.read() - 1);
      }

      // Mostrar por Serial para debug
      Serial.print("Servo horizontal: ");
      Serial.print(myServo.read());
      Serial.print(" | Servo vertical1: ");
      Serial.print(alturaServo.read());
      Serial.print(" | Servo vertical2: ");
      Serial.println(altura2Servo.read());

      // Retardo leve para no mover demasiado rápido
      delay(10);
    }
  }
}

// Maneja la apertura/cierre de la garra al pulsar el botón
void handleGarra() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(SWPin);

  // Transición de HIGH a LOW => el botón se ha pulsado
  if (lastButtonState == HIGH && buttonState == LOW) {
    garraAbierta = !garraAbierta; // Cambia el estado de la garra

    if (garraAbierta) {
      garraServo.write(garraPositionOpen);
      Serial.println("Garra abierta");
    } else {
      garraServo.write(garraPositionClosed);
      Serial.println("Garra cerrada");
    }
    delay(200); // Pequeño delay para debounce
  }
  lastButtonState = buttonState;
}
