#ifndef MIMIC_H_INCL
#define MIMIC_H_INCL

// ------------------------------------------------------------------------
// Magic numbers and helpful macros

enum LEDCOLOR { OFF, RED, GREEN, ORANGE };

enum Mode { MIMIC, IDLE };

#ifndef UNUSED
#define UNUSED(var) do { (void) var; } while (0);
#endif

void flashLED(int color, int color2 = OFF, int count = 5, int timing = 200, bool restore = false);


// The AppState structure is used to hold various program state values
// 
struct AppState {
  unsigned
    ledColor      :  2,
    mode          :  1,
    stopPlayback  :  1,
    playbackPause : 13;

  AppState() {
    ledColor = OFF;
    mode = IDLE;
    stopPlayback = 0;
    playbackPause = 400;
  }
};


// The SerialPacket structure is used to hold Serial API command packets
// 
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
    pinch : 12, /* 0 - 4095 values */
    wrist : 12,
    elbow : 12,
    waist : 12;

  Pos() {
    pinch = wrist = elbow = waist = 0;
  }

  Pos(int pinchVal, int wristVal, int elbowVal, int waistVal) {
    pinch = pinchVal;
    wrist = wristVal;
    elbow = elbowVal;
    waist = waistVal;
  }

  Pos operator + (const Pos &rhs) {
    return Pos(pinch + rhs.pinch, wrist + rhs.wrist, elbow + rhs.elbow, waist + rhs.waist);
  }

  Pos operator - (const Pos &rhs) {
    return Pos(pinch - rhs.pinch, wrist - rhs.wrist, elbow - rhs.elbow, waist - rhs.waist);
  }

  Pos & operator += (const Pos &rhs) {
    pinch += rhs.pinch;
    wrist += rhs.wrist;
    elbow += rhs.elbow;
    waist += rhs.waist;

    return *this;
  }

  Pos & operator -= (const Pos &rhs) {
    pinch -= rhs.pinch;
    wrist -= rhs.wrist;
    elbow -= rhs.elbow;
    waist -= rhs.waist;

    return *this;
  }

  unsigned int maxDelta(const Pos &rhs) {
    Pos delta = operator - (rhs);
    unsigned int result = max(abs(delta.pinch), abs(delta.wrist));
    result = max(result, abs(delta.elbow));
    result = max(result, abs(delta.waist));

    return result;
  }
};


// forward declaration for structure not defined until later
struct Limits;

// The Arm structure is used to hold the four values for an arm
// and to be able to clip the values to their allowed ranges
struct Arm : public Pos {
  Arm() : Pos() {
  }

  Arm(int pinchVal, int wristVal, int elbowVal, int waistVal) : Pos(pinchVal, wristVal, elbowVal, waistVal) {
  }

  Arm(const Arm &rhs) : Pos(rhs.pinch, rhs.wrist, rhs.elbow, rhs.waist)  {
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
  
  Arm &clip(Pos &limit1, Pos &limit2) {
    pinch = clip(pinch, limit1.pinch, limit2.pinch);
    wrist = clip(wrist, limit1.wrist, limit2.wrist);
    elbow = clip(elbow, limit1.elbow, limit2.elbow);
    waist = clip(waist, limit1.waist, limit2.waist);

    return *this;
  }

  Arm &clip(Arm &limit1, Arm &limit2) {
    return clip(dynamic_cast<Pos&>(limit1), dynamic_cast<Pos&>(limit2));
  }

  Arm &clip(Limits &limits);
};


// the Limits structure holds the beginning
// and ending range for each value.  Used to
// clip the values to their allowed ranges.
//
struct Limits {
  Pos a, b;

  Limits() {
  }

  Limits(Pos &limit1, Pos &limit2) : a(limit1), b(limit2) {
  }

  Limits(Arm &limit1, Arm &limit2) : a(limit1), b(limit2) {
  }
};


Arm &Arm::clip(Limits &limits) {
  return clip(limits.a, limits.b);
}



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

  int foreach(int (*func)(T &)) {
    Node<T> *p = head;
    while (p != nullptr) {
      if (func(p->t) != 0)
        return 1;
      p = p->next;
    }
    return 0;
  }
};

#endif // #ifndef MIMIC_H_INCL
