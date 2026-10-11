#include "json3.h"

#define TRUE 1
#define FALSE 0
#define __ -1 /* the universal error code */

/*
    Characters are mapped into these 31 character classes. This allows for
    a significant reduction in the size of the state transition table.
*/
enum classes {
  C_SPACE, /* space */
  C_WHITE, /* other whitespace */
  C_LCURB, /* {  */
  C_RCURB, /* } */
  C_LSQRB, /* [ */
  C_RSQRB, /* ] */
  C_COLON, /* : */
  C_COMMA, /* , */
  C_QUOTE, /* " */
  C_BACKS, /* \ */
  C_SLASH, /* / */
  C_PLUS,  /* + */
  C_MINUS, /* - */
  C_POINT, /* . */
  C_ZERO,  /* 0 */
  C_DIGIT, /* 123456789 */
  C_LOW_A, /* a */
  C_LOW_B, /* b */
  C_LOW_C, /* c */
  C_LOW_D, /* d */
  C_LOW_E, /* e */
  C_LOW_F, /* f */
  C_LOW_L, /* l */
  C_LOW_N, /* n */
  C_LOW_R, /* r */
  C_LOW_S, /* s */
  C_LOW_T, /* t */
  C_LOW_U, /* u */
  C_ABCDF, /* ABCDF */
  C_E,     /* E */
  C_ETC,   /* everything else */
  NR_CLASSES
};

static int ascii_class[128] = {
/*
    This array maps the 128 ASCII characters into character classes.
    The remaining Unicode characters should be mapped to C_ETC.
    Non-whitespace control characters are errors.
*/
    __,      __,      __,      __,      __,      __,      __,      __,
    __,      C_WHITE, C_WHITE, __,      __,      C_WHITE, __,      __,
    __,      __,      __,      __,      __,      __,      __,      __,
    __,      __,      __,      __,      __,      __,      __,      __,

    C_SPACE, C_ETC,   C_QUOTE, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_PLUS,  C_COMMA, C_MINUS, C_POINT, C_SLASH,
    C_ZERO,  C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT,
    C_DIGIT, C_DIGIT, C_COLON, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,

    C_ETC,   C_ABCDF, C_ABCDF, C_ABCDF, C_ABCDF, C_E,     C_ABCDF, C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_LSQRB, C_BACKS, C_RSQRB, C_ETC,   C_ETC,

    C_ETC,   C_LOW_A, C_LOW_B, C_LOW_C, C_LOW_D, C_LOW_E, C_LOW_F, C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_LOW_L, C_ETC,   C_LOW_N, C_ETC,
    C_ETC,   C_ETC,   C_LOW_R, C_LOW_S, C_LOW_T, C_LOW_U, C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_LCURB, C_ETC,   C_RCURB, C_ETC,   C_ETC
};

/*
    The state codes.
*/
enum states {
  GO, /* start    */
  OK, /* ok       */
  OB, /* object   */
  KE, /* key      */
  CO, /* colon    */
  VA, /* value    */
  AR, /* array    */
  ST, /* string   */
  ES, /* escape   */
  U1, /* u1       */
  U2, /* u2       */
  U3, /* u3       */
  U4, /* u4       */
  MI, /* minus    */
  ZE, /* zero     */
  IN, /* integer  */
  FR, /* fraction */
  FS, /* fraction */
  E1, /* e        */
  E2, /* ex       */
  E3, /* exp      */
  T1, /* tr       */
  T2, /* tru      */
  T3, /* true     */
  F1, /* fa       */
  F2, /* fal      */
  F3, /* fals     */
  F4, /* false    */
  N1, /* nu       */
  N2, /* nul      */
  N3, /* null     */
  NR_STATES
};

static const int state_transition_table[NR_STATES][NR_CLASSES] = {
    /*
        The state transition table takes the current state and the current
        symbol, and returns either a new state or an action. An action is
        represented as a negative number. A JSON text is accepted if at
        the end of the text the state is OK and if the mode is MODE_DONE.
    */
    /*start GO */ {GO,GO,-6,__,-5,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*ok OK    */ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*object OB*/ {OB,OB,__,-9,__,__,__,__,ST,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*key KE   */ {KE,KE,__,__,__,__,__,__,ST,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*colon CO */ {CO,CO,__,__,__,__,-2,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*value VA */ {VA,VA,-6,__,-5,__,__,__,ST,__,__,__,MI,__,ZE,IN,__,__,__,__,__,F1,__,N1,__,__,T1,__,__,__,__},
    /*array AR */ {AR,AR,-6,__,-5,-7,__,__,ST,__,__,__,MI,__,ZE,IN,__,__,__,__,__,F1,__,N1,__,__,T1,__,__,__,__},
    /*string ST*/ {ST,__,ST,ST,ST,ST,ST,ST,-4,ES,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST},
    /*escape ES*/ {__,__,__,__,__,__,__,__,ST,ST,ST,__,__,__,__,__,__,ST,__,__,__,ST,__,ST,ST,__,ST,U1,__,__,__},
    /*u1 U1    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U2,U2,U2,U2,U2,U2,U2,U2,__,__,__,__,__,__,U2,U2,__},
    /*u2 U2    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U3,U3,U3,U3,U3,U3,U3,U3,__,__,__,__,__,__,U3,U3,__},
    /*u3 U3    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U4,U4,U4,U4,U4,U4,U4,U4,__,__,__,__,__,__,U4,U4,__},
    /*u4 U4    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,ST,ST,ST,ST,ST,ST,ST,ST,__,__,__,__,__,__,ST,ST,__},
    /*minus MI */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,ZE,IN,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*zero ZE  */ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,FR,__,__,__,__,__,__,E1,__,__,__,__,__,__,__,__,E1,__},
    /*int IN   */ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,FR,IN,IN,__,__,__,__,E1,__,__,__,__,__,__,__,__,E1,__},
    /*frac FR  */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,FS,FS,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*fracs FS */ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,__,FS,FS,__,__,__,__,E1,__,__,__,__,__,__,__,__,E1,__},
    /*e E1     */ {__,__,__,__,__,__,__,__,__,__,__,E2,E2,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*ex E2    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*exp E3   */ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*tr T1    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T2,__,__,__,__,__,__},
    /*tru T2   */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T3,__,__,__},
    /*true T3  */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,OK,__,__,__,__,__,__,__,__,__,__},
    /*fa F1    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F2,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
    /*fal F2   */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F3,__,__,__,__,__,__,__,__},
    /*fals F3  */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F4,__,__,__,__,__},
    /*false F4 */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,OK,__,__,__,__,__,__,__,__,__,__},
    /*nu N1    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N2,__,__,__},
    /*nul N2   */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N3,__,__,__,__,__,__,__,__},
    /*null N3  */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,OK,__,__,__,__,__,__,__,__},
};

/*
    These are the modes of the currently open container. While a container
    token is unfinished, its `size` field stores the mode and its `end`
    field counts the children seen so far; both receive their final values
    when the container is closed. An object key string sitting on the
    stack top (waiting for its value) acts as MODE_OBJECT.
*/
enum modes { MODE_ARRAY, MODE_KEY, MODE_OBJECT };

/*
    The mode of the stack-top token. A key string on the stack acts as
    MODE_OBJECT: it is only pushed while waiting for its value, which
    is exactly when the enclosing object is in object mode.
*/
static int token_mode(const json3_token_t *tok) {
  if (tok->type == JSON3_STRING) {
    return MODE_OBJECT;
  }
  return tok->size;
}

static int top_mode(const json3_parser_t *parser) {
  if (parser->toksuper < 0) {
    return __;
  }
  return token_mode(&parser->tokens[parser->toksuper]);
}

/*
    The mode the container-close actions check against. When a value in
    an object has just been completed, the stack top is still the key
    string whose enclosing object is the real target, so look through
    it to its parent.
*/
static int close_target_mode(const json3_parser_t *parser) {
  const json3_token_t *top;
  if (parser->toksuper < 0) {
    return __;
  }
  top = &parser->tokens[parser->toksuper];
  if (top->type == JSON3_STRING && top->parent >= 0) {
    return token_mode(&parser->tokens[top->parent]);
  }
  return token_mode(top);
}

/*
    A value token was just completed: account for it in the parent and
    restore the stack top. If the parent is a key string, mark the key as
    having its value (size 1) and pop back to the enclosing container;
    the pair counts as one child of that container. Otherwise bump the
    parent's child count directly. The child count lives in the
    container's scratch `end` field until it is closed.
*/
static void complete_value(json3_parser_t *parser, int index) {
  int parent = parser->tokens[index].parent;
  if (parent >= 0 && parser->tokens[parent].type == JSON3_STRING) {
    parser->tokens[parent].size = 1;
    parent = parser->tokens[parent].parent;
  }
  if (parent >= 0) {
    parser->tokens[parent].end++;
  }
  parser->toksuper = parent;
}

/*
    Close the topmost container token: `size` receives the child count
    that was kept in the scratch `end` field, and `end` receives the
    final boundary. Then account for the container in its own parent.
*/
static void close_container(json3_parser_t *parser) {
  int index = parser->toksuper;
  if (parser->tokens[index].type == JSON3_STRING) {
    /* A key string: the enclosing container is its parent. */
    index = parser->tokens[index].parent;
  }
  parser->tokens[index].size = parser->tokens[index].end;
  parser->tokens[index].end = parser->pos + 1;
  complete_value(parser, index);
}

/*
    Allocate a token from the caller-provided buffer. Returns its index,
    or JSON_SCANNER_ERROR_NOMEM if the buffer is exhausted. Tokens are
    allocated in input order and are never reused, which is what makes
    the parent-linked chain usable as the parser stack.
*/
static int alloc_token(json3_parser_t *parser, json3_token_type type,
                       int start) {
  json3_token_t *tok;
  unsigned int index;
  if (parser->toknext >= parser->num_tokens) {
    return JSON3_PARSER_ERROR_NOMEM;
  }
  index = parser->toknext++;
  tok = &parser->tokens[index];
  tok->type = type;
  tok->start = start;
  tok->end = -1;
  tok->size = 0;
  tok->parent = parser->toksuper;
  return (int)index;
}

/*
    Push a container token onto the stack. Its `size` field stores the
    mode and its `end` field counts children until the container closes.
*/
static int push(json3_parser_t *parser, json3_token_type type,
                          int mode, int start) {
  int index = alloc_token(parser, type, start);
  if (index < 0) {
    return index;
  }
  parser->tokens[index].size = mode;
  parser->tokens[index].end = 0;
  parser->toksuper = index;
  return 0;
}

static int reject(json3_parser_t *parser) {
  if (parser->error == 0) {
    parser->error = JSON3_PARSER_ERROR_INVAL;
  }
  return FALSE;
}

void json3_parser_init(json3_parser_t *parser, json3_token_t *tokens,
                       unsigned int num_tokens) {
  parser->tokens = tokens;
  parser->num_tokens = num_tokens;
  parser->toknext = 0;
  parser->toksuper = -1;
  parser->state = GO;
  parser->pos = 0;
  parser->error = 0;
}

int json3_parser_feed(json3_parser_t *parser, code_point cp) {
  int next_class, next_state;

  if (parser->error != 0) {
    return FALSE;
  }
  /*
      Determine the character's class. Code points at or above U+0080 are
      only valid inside strings and map to C_ETC, like in JSON_checker.
  */
  if (cp >= 128) {
    next_class = C_ETC;
  } else {
    next_class = ascii_class[cp];
    if (next_class <= __) {
      return reject(parser);
    }
  }
  /*
      Get the next state from the state transition table.
  */
  next_state = state_transition_table[parser->state][next_class];
  if (next_state >= 0) {
    /*
        Change the state, allocating tokens where the state machine
        shows that a token starts or ends.
    */
    switch (next_state) {
    case ST:
      switch (parser->state) {
      case OB:
      case KE:
      case VA:
      case AR:
        /* An opening quote: the string starts after it. */
        if (alloc_token(parser, JSON3_STRING, parser->pos + 1) < 0) {
          parser->error = JSON3_PARSER_ERROR_NOMEM;
          return FALSE;
        }
        break;
      default:
        break;
      }
      break;
    case MI:
    case ZE:
    case IN:
    case T1:
    case F1:
    case N1:
      if (parser->state == VA || parser->state == AR) {
        if (alloc_token(parser, JSON3_PRIMITIVE, parser->pos) < 0) {
          parser->error = JSON3_PARSER_ERROR_NOMEM;
          return FALSE;
        }
      }
      break;
    case OK:
      switch (parser->state) {
      case ZE:
      case IN:
      case FS:
      case E3:
        /* Number terminated by whitespace: it ends before this point. */
        parser->tokens[parser->toknext - 1].end = parser->pos;
        complete_value(parser, parser->toknext - 1);
        break;
      case T3:
      case F4:
      case N3:
        /* true/false/null end with this code point. */
        parser->tokens[parser->toknext - 1].end = parser->pos + 1;
        complete_value(parser, parser->toknext - 1);
        break;
      default:
        break;
      }
      break;
    default:
      break;
    }
    parser->state = next_state;
    /*
        Or perform one of the actions.
    */
  } else {
    switch (next_state) {
    /* empty } */ case -9:
      if (top_mode(parser) != MODE_KEY) {
        return reject(parser);
      }
      close_container(parser);
      parser->state = OK;
      break;

    /* } */ case -8:
      /* An open number ends right before this code point. */
      if (parser->state == ZE || parser->state == IN ||
          parser->state == FS || parser->state == E3) {
        parser->tokens[parser->toknext - 1].end = parser->pos;
        complete_value(parser, parser->toknext - 1);
      }
      if (close_target_mode(parser) != MODE_OBJECT) {
        return reject(parser);
      }
      close_container(parser);
      parser->state = OK;
      break;

    /* ] */ case -7:
      /* An open number ends right before this code point. */
      if (parser->state == ZE || parser->state == IN ||
          parser->state == FS || parser->state == E3) {
        parser->tokens[parser->toknext - 1].end = parser->pos;
        complete_value(parser, parser->toknext - 1);
      }
      if (close_target_mode(parser) != MODE_ARRAY) {
        return reject(parser);
      }
      close_container(parser);
      parser->state = OK;
      break;

    /* { */ case -6:
      if (push(parser, JSON3_OBJECT, MODE_KEY,
                         parser->pos) < 0) {
        parser->error = JSON3_PARSER_ERROR_NOMEM;
        return FALSE;
      }
      parser->state = OB;
      break;

    /* [ */ case -5:
      if (push(parser, JSON3_ARRAY, MODE_ARRAY,
                         parser->pos) < 0) {
        parser->error = JSON3_PARSER_ERROR_NOMEM;
        return FALSE;
      }
      parser->state = AR;
      break;

    /* " */ case -4:
      /* The closing quote ends the current string token. */
      parser->tokens[parser->toknext - 1].end = parser->pos;
      switch (top_mode(parser)) {
      case MODE_KEY:
        /*
            A string in key mode is an object key: it awaits a colon.
        */
        parser->state = CO;
        break;
      case MODE_ARRAY:
      case MODE_OBJECT:
        /*
            A string in array or object mode is a complete value.
        */
        complete_value(parser, parser->toknext - 1);
        parser->state = OK;
        break;
      default:
        return reject(parser);
      }
      break;

    /* , */ case -3:
      /* An open number ends right before this code point. */
      if (parser->state == ZE || parser->state == IN ||
          parser->state == FS || parser->state == E3) {
        parser->tokens[parser->toknext - 1].end = parser->pos;
        complete_value(parser, parser->toknext - 1);
      }
      switch (top_mode(parser)) {
      case MODE_OBJECT:
        /*
            A comma causes a flip from object mode to key mode.
        */
        {
          int index = parser->toksuper;
          if (parser->tokens[index].type == JSON3_STRING) {
            /* A completed value: the key string is still on the stack. */
            index = parser->tokens[index].parent;
          }
          parser->tokens[index].size = MODE_KEY;
        }
        parser->state = KE;
        break;
      case MODE_ARRAY:
        parser->state = VA;
        break;
      default:
        return reject(parser);
      }
      break;

    /* : */ case -2:
      /*
          A colon flips the enclosing object from key mode to object
          mode: the just-completed key string goes on the stack, where
          it acts as MODE_OBJECT until its value is complete.
      */
      if (top_mode(parser) != MODE_KEY) {
        return reject(parser);
      }
      parser->tokens[parser->toksuper].size = MODE_OBJECT;
      parser->toksuper = parser->toknext - 1;
      parser->state = VA;
      break;

    /*
        Bad action.
    */
    default:
      return reject(parser);
    }
  }
  parser->pos++;
  return TRUE;
}

int json3_parser_done(json3_parser_t *parser) {
  if (parser->error != 0) {
    return parser->error;
  }
  /*
      A top-level number can only be terminated by end of input. Its end
      boundary is also only known now, when the stream is finished.
  */
  switch (parser->state) {
  case ZE:
  case IN:
  case FS:
  case E3:
    parser->tokens[parser->toknext - 1].end = parser->pos;
    complete_value(parser, parser->toknext - 1);
    parser->state = OK;
    break;
  default:
    break;
  }
  if (parser->state != OK || parser->toksuper != -1) {
    parser->error = JSON3_PARSER_ERROR_PART;
    return parser->error;
  }
  return (int)parser->toknext;
}
