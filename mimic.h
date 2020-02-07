#ifndef MIMIC_H_INCL
#define MIMIC_H_INCL

// ------------------------------------------------------------------------
// Magic numbers and helpful macros

enum LedColor { OFF, RED, GREEN, ORANGE };
enum Mode { MIMIC, IDLE };

#ifndef UNUSED
#define UNUSED(var) do { (void) var; } while (0);
#endif

void flashLED(LedColor color, LedColor color2 = OFF, int count = 5, int timing = 200, bool restore = false);

// The AppState structure is used to hold various program state values
// 
struct AppState {
  unsigned
    ledColor      :  2,
    mode          :  1,
    stopPlayback  :  1,
    playbackPause : 12;

  AppState() {
    ledColor = OFF;
    mode = IDLE;
    stopPlayback = 0;
    playbackPause = 400;
  }
};


// The SerialPacket structure is used to hold Serial API command packets
union SerialPacket {
  struct {
    byte cmd;
    int value;
  } fields;
  byte data[sizeof(fields)];
};


// The Pos structure is used to hold the four values for a specific arm position
struct Pos {
  unsigned
    pinch : 12, /* 0 - 4095 values. Adjust as needed */
    wrist : 12,
    elbow : 12,
    waist : 12;

  Pos() {
    pinch = wrist = elbow = waist = 0;
  }

  Pos(int pinchVal, int wristVal, int elbowVal, int waistVal) :
    pinch(pinchVal),
    wrist(wristVal),
    elbow(elbowVal),
    waist(waistVal) {
  }
};


// The Limits structure holds the beginning
// and ending range for each value.  Used to
// clip the values to their allowed ranges.
//
struct Limits {
  Pos a, b;

  Limits() {
  }

  Limits(Pos &limit1, Pos &limit2) : a(limit1), b(limit2) {
  }
};


// The Arm structure is used to represent and input or output arm
// It can hold 4 values which represent either the input values
// or the output values depending on use.
// It can clip the values to their allowed ranges
// It can store the pins used to interface with the reading or writing of the arm values
// 
struct Arm : public Pos {
  unsigned pinchPin, wristPin, elbowPin, waistPin;
  Limits range;

  Arm() = delete;

  Arm(int pinch_pin, int wrist_pin, int elbow_pin, int waist_pin, Limits &limits) :
    pinchPin(pinch_pin),
    wristPin(wrist_pin),
    elbowPin(elbow_pin),
    waistPin(waist_pin),
    range(limits) {
  }

  static int16_t clip(int16_t value, int16_t limit1, int16_t limit2) {
    int16_t minVal = min(limit1, limit2);
    int16_t maxVal = max(limit1, limit2);
  
    if (value < minVal)
      value = minVal;
    else if (value > maxVal)
      value = maxVal;
  
    return value;
  }
};


// The Node class is the base entry for a singly or doubly linked list
template <class T>
struct Node {
  T t;
  Node<T> *prev, *next;

  Node(T r) {
    t = r;
    prev = nullptr;
    next = nullptr;
  }

  Node(T r, Node *prv, Node *nxt) {
    t = r;
    next = nxt;
    prev = prv;
  }

  T & operator * () {
    return t;
  }
};


// The LinkedList class allows for storing a linked list of any object type
// 
template <class T>
struct LinkedList {
  Node<T> *head, *tail;

  LinkedList() : head(nullptr), tail(nullptr) {
  }

  LinkedList(T r) : head(new Node<T>(r, nullptr, nullptr)), tail(head) {
  }

  virtual ~LinkedList() {
    clear();
  }

  bool empty() {
    return head == nullptr && tail == nullptr;
  }

  void clear() {
    while (head != nullptr) {
      head = removeHead();
    }
    tail = nullptr;
  }

  Node<T> *addTail(T r) {
    tail = new Node<T>(r, tail, nullptr);
    if (head == nullptr) {
      head = tail;
    }
    if (tail->prev != nullptr) {
      tail->prev->next = tail;
    }
    return tail;
  }

  Node<T> *addHead(T r) {
    head = new Node<T>(r, nullptr, head);
    if (tail == nullptr) {
      tail = head;
    }
    if (head->next != nullptr) {
      head->next->prev = head;
    }
    return head;
  }

  Node<T> *removeTail() {
    if (tail != nullptr) {
      Node<T> *tmp = tail;
      if (tail->prev != nullptr) {
        tail->prev->next = tail->next;
      }
      tail = tail->prev;
      delete tmp;
      if (head == tmp) {
        head = nullptr;
      }
    }
    return tail;
  }

  Node<T> *removeHead() {
    if (head != nullptr) {
      Node<T> *tmp = head;
      if (head->next != nullptr) {
        head->next->prev = head->prev;
      }
      head = head->next;
      delete tmp;
      if (tail == tmp) {
        tail = nullptr;
      }
    }
    return head;
  }

};

#endif // #ifndef MIMIC_H_INCL
