#ifndef MIMIC_H_INCL
#define MIMIC_H_INCL

// ------------------------------------------------------------------------
// Magic numbers and helpful macros

enum LEDCOLOR { OFF, RED, GREEN, ORANGE };

#ifndef UNUSED
#define UNUSED(var) do { (void) var; } while (0);
#endif

// forward declaration for structure not defined until later
struct Limits;

// The Pos structure is used to hold the four values for an arm
struct Pos {
  unsigned pinch, wrist, elbow, waist;
};

// The Arm structure is used to hold the four values for an arm
// and to be able to clip the values to their allowed ranges
struct Arm : public Pos {
  Arm() {
    pinch = wrist = elbow = waist = 0;
  }

  Arm(int pinchVal, int wristVal, int elbowVal, int waistVal) {
    pinch = pinchVal;
    wrist = wristVal;
    elbow = elbowVal;
    waist = waistVal;
  }

  Arm(const Arm &rhs) {
    pinch = rhs.pinch;
    wrist = rhs.wrist;
    elbow = rhs.elbow;
    waist = rhs.waist;
  }

  static uint16_t clip(uint16_t value, uint16_t limit1, uint16_t limit2) {
    uint16_t minVal = min(limit1, limit2);
    uint16_t maxVal = max(limit1, limit2);
  
    if (value < minVal)
      value = minVal;
    else if (value > maxVal)
      value = maxVal;
  
    return value;
  }
  
  Arm &clip(Arm &limit1, Arm &limit2) {
    pinch = clip(pinch, limit1.pinch, limit2.pinch);
    wrist = clip(wrist, limit1.wrist, limit2.wrist);
    elbow = clip(elbow, limit1.elbow, limit2.elbow);
    waist = clip(waist, limit1.waist, limit2.waist);

    return *this;
  }

  Arm &clip(Limits &limits);
};


// the Limits structure holds the beginning
// and ending range for each value.  Used to
// clip the values to their allowed ranges.
//
struct Limits {
  Arm a, b;

  Limits() {
  }

  Limits(Arm &limit1, Arm &limit2) : a(limit1), b(limit2) {
  }
};


Arm &Arm::clip(Limits &limits) {
  return clip(limits.a, limits.b);
}



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


template <class T>
struct Tree {
  Node<T> *head, *tail;

  Tree() : head(nullptr), tail(nullptr) {
  }

  Tree(T r) : head(new Node<T>(r, nullptr, nullptr)), tail(head) {
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

  void clear() {
    while (head != nullptr) {
      head = removeHead();
    }
    tail = nullptr;
  }

  bool empty() {
    return head == nullptr && tail == nullptr;
  }

  void foreach(void (*func)(T &)) {
    Node<T> *p = head;
    while (p != nullptr) {
      func(p->t);
      p = p->next;
    }
  }
};

#endif // #ifndef MIMIC_H_INCL
