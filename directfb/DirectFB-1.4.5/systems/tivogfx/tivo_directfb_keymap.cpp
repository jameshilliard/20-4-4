////////////////////////////////////////////////////////////////////////////////
//
// File: tivo_directfb_keymap.cpp
//
// Copyright 2014 TiVo Inc.  All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "tivo_directfb_keymap.h"
#include <stx/StxIpcProtocol.h>
#include <tmk/TmkDebugPrint.h>

SETUP_DEBUG_ENV( "tivo_directfb_keymap" );

//####################################################################

/**
 * TiVoToDfbKeyIdMapping
 *
 * Map from the StxRemoteKey (TvRemote) namespace to the
 * DFBInputDeviceKeyIdentifier namespace.
 */
struct TiVoToDfbKeyIdMapping
{
    U32                         tivoKey;
    DFBInputDeviceKeyIdentifier dfbKeyId;
    DFBInputDeviceKeySymbol     dfbKeySymbol;
};


/**
 * ShiftMapping
 *
 * Map from a DFBInputDeviceKeySymbol to the symbol that should be used if
 * shift is pressed.
 **/
struct ShiftMapping
{
    DFBInputDeviceKeySymbol unshifted, shifted;
};


//####################################################################
// following is the last custom key used 
// STX_REMOTE_KEY_SEARCH,  DIKI_UNKNOWN,   DIKS_CUSTOM43
//

/**
 * tivoToDfbKeyIdMappingsG
 */
static const TiVoToDfbKeyIdMapping tivoToDfbKeyIdMappingsG[] =
{
    //-----------------------------------------------------
    {  STX_REMOTE_KEY_UP,                          DIKI_UP,              DIKS_CURSOR_UP    },
    {  STX_REMOTE_KEY_DOWN,                        DIKI_DOWN,            DIKS_CURSOR_DOWN  },
    {  STX_REMOTE_KEY_LEFT,                        DIKI_LEFT,            DIKS_CURSOR_LEFT  },
    {  STX_REMOTE_KEY_RIGHT,                       DIKI_RIGHT,           DIKS_CURSOR_RIGHT },
    {  STX_REMOTE_KEY_SELECT,                      DIKI_ENTER,           DIKS_SELECT       },
    {  STX_REMOTE_KEY_TIVO,                        DIKI_UNKNOWN,         DIKS_CUSTOM0      },
    {  STX_REMOTE_KEY_LIVETV,                      DIKI_UNKNOWN,         DIKS_CUSTOM1      },
    {  STX_REMOTE_KEY_THUMBSUP,                    DIKI_UNKNOWN,         DIKS_CUSTOM10     },
    {  STX_REMOTE_KEY_THUMBSDOWN,                  DIKI_UNKNOWN,         DIKS_CUSTOM11     },
    {  STX_REMOTE_KEY_CHANNELUP,                   DIKI_UNKNOWN,         DIKS_CHANNEL_UP   },
    {  STX_REMOTE_KEY_CHANNELDOWN,                 DIKI_UNKNOWN,         DIKS_CHANNEL_DOWN },
    {  STX_REMOTE_KEY_RECORD,                      DIKI_UNKNOWN,         DIKS_RECORD       },
    {  STX_REMOTE_KEY_DISPLAY,                     DIKI_UNKNOWN,         DIKS_INFO         }, // NOTE: For all directfb apps, the DISPLAY key is remapped to INFO
    {  STX_REMOTE_KEY_NUM0,                        DIKI_0,               DIKS_0            },
    {  STX_REMOTE_KEY_NUM1,                        DIKI_1,               DIKS_1            },
    {  STX_REMOTE_KEY_NUM2,                        DIKI_2,               DIKS_2            },
    {  STX_REMOTE_KEY_NUM3,                        DIKI_3,               DIKS_3            },
    {  STX_REMOTE_KEY_NUM4,                        DIKI_4,               DIKS_4            },
    {  STX_REMOTE_KEY_NUM5,                        DIKI_5,               DIKS_5            },
    {  STX_REMOTE_KEY_NUM6,                        DIKI_6,               DIKS_6            },
    {  STX_REMOTE_KEY_NUM7,                        DIKI_7,               DIKS_7            },
    {  STX_REMOTE_KEY_NUM8,                        DIKI_8,               DIKS_8            },
    {  STX_REMOTE_KEY_NUM9,                        DIKI_9,               DIKS_9            },
    {  STX_REMOTE_KEY_ENTER,                       DIKI_ENTER,           DIKS_ENTER        },
    {  STX_REMOTE_KEY_CLEAR,                       DIKI_UNKNOWN,         DIKS_CLEAR        },
    {  STX_REMOTE_KEY_PLAY,                        DIKI_UNKNOWN,         DIKS_PLAY         },
    {  STX_REMOTE_KEY_PAUSE,                       DIKI_PAUSE,           DIKS_PLAYPAUSE    },
    {  STX_REMOTE_KEY_SLOW,                        DIKI_UNKNOWN,         DIKS_SLOW         },
    {  STX_REMOTE_KEY_FORWARD,                     DIKI_UNKNOWN,         DIKS_FASTFORWARD  },
    {  STX_REMOTE_KEY_REVERSE,                     DIKI_UNKNOWN,         DIKS_REWIND       },
    {  STX_REMOTE_KEY_STANDBY,                     DIKI_UNKNOWN,         DIKS_CUSTOM3      },
    {  STX_REMOTE_KEY_NOWSHOWING,                  DIKI_UNKNOWN,         DIKS_CUSTOM4      },
    {  STX_REMOTE_KEY_REPLAY,                      DIKI_UNKNOWN,         DIKS_PREVIOUS     },
    {  STX_REMOTE_KEY_ADVANCE,                     DIKI_UNKNOWN,         DIKS_NEXT         },
    {  STX_REMOTE_KEY_DELIMITER,                   DIKI_UNKNOWN,         DIKS_CUSTOM12     },
    {  STX_REMOTE_KEY_GUIDE,                       DIKI_UNKNOWN,         DIKS_EPG          },
    {  STX_REMOTE_KEY_TVPOWER,                     DIKI_UNKNOWN,         DIKS_POWER        },
    {  STX_REMOTE_KEY_VOLUMEUP,                    DIKI_UNKNOWN,         DIKS_VOLUME_UP    },
    {  STX_REMOTE_KEY_VOLUMEDOWN,                  DIKI_UNKNOWN,         DIKS_VOLUME_DOWN  },
    {  STX_REMOTE_KEY_MUTE,                        DIKI_UNKNOWN,         DIKS_MUTE         },
    {  STX_REMOTE_KEY_TVINPUT,                     DIKI_UNKNOWN,         DIKS_CUSTOM5      },
    {  STX_REMOTE_KEY_INFO,                        DIKI_UNKNOWN,         DIKS_INFO         },
    {  STX_REMOTE_KEY_WINDOW,                      DIKI_UNKNOWN,         DIKS_ZOOM         },
    {  STX_REMOTE_KEY_EXIT,                        DIKI_UNKNOWN,         DIKS_EXIT         },
    {  STX_REMOTE_KEY_VIDEO_FORMAT,                DIKI_UNKNOWN,         DIKS_MODE         },
    {  STX_REMOTE_KEY_A,                           DIKI_A,               DIKS_SMALL_A      },
    {  STX_REMOTE_KEY_B,                           DIKI_B,               DIKS_SMALL_B      },
    {  STX_REMOTE_KEY_C,                           DIKI_C,               DIKS_SMALL_C      },
    {  STX_REMOTE_KEY_D,                           DIKI_D,               DIKS_SMALL_D      },
    {  STX_REMOTE_KEY_E,                           DIKI_E,               DIKS_SMALL_E      },
    {  STX_REMOTE_KEY_F,                           DIKI_F,               DIKS_SMALL_F      },
    {  STX_REMOTE_KEY_G,                           DIKI_G,               DIKS_SMALL_G      },
    {  STX_REMOTE_KEY_H,                           DIKI_H,               DIKS_SMALL_H      },
    {  STX_REMOTE_KEY_I,                           DIKI_I,               DIKS_SMALL_I      },
    {  STX_REMOTE_KEY_J,                           DIKI_J,               DIKS_SMALL_J      },
    {  STX_REMOTE_KEY_K,                           DIKI_K,               DIKS_SMALL_K      },
    {  STX_REMOTE_KEY_L,                           DIKI_L,               DIKS_SMALL_L      },
    {  STX_REMOTE_KEY_M,                           DIKI_M,               DIKS_SMALL_M      },
    {  STX_REMOTE_KEY_N,                           DIKI_N,               DIKS_SMALL_N      },
    {  STX_REMOTE_KEY_O,                           DIKI_O,               DIKS_SMALL_O      },
    {  STX_REMOTE_KEY_P,                           DIKI_P,               DIKS_SMALL_P      },
    {  STX_REMOTE_KEY_Q,                           DIKI_Q,               DIKS_SMALL_Q      },
    {  STX_REMOTE_KEY_R,                           DIKI_R,               DIKS_SMALL_R      },
    {  STX_REMOTE_KEY_S,                           DIKI_S,               DIKS_SMALL_S      },
    {  STX_REMOTE_KEY_T,                           DIKI_T,               DIKS_SMALL_T      },
    {  STX_REMOTE_KEY_U,                           DIKI_U,               DIKS_SMALL_U      },
    {  STX_REMOTE_KEY_V,                           DIKI_V,               DIKS_SMALL_V      },
    {  STX_REMOTE_KEY_W,                           DIKI_W,               DIKS_SMALL_W      },
    {  STX_REMOTE_KEY_X,                           DIKI_X,               DIKS_SMALL_X      },
    {  STX_REMOTE_KEY_Y,                           DIKI_Y,               DIKS_SMALL_Y      },
    {  STX_REMOTE_KEY_Z,                           DIKI_Z,               DIKS_SMALL_Z      },
    {  STX_REMOTE_KEY_MINUS,                       DIKI_MINUS_SIGN,      DIKS_MINUS_SIGN   },
    {  STX_REMOTE_KEY_EQUALS,                      DIKI_EQUALS_SIGN,     DIKS_EQUALS_SIGN  },
    {  STX_REMOTE_KEY_LBRACKET,                    DIKI_BRACKET_LEFT,    DIKS_SQUARE_BRACKET_LEFT  },
    {  STX_REMOTE_KEY_RBRACKET,                    DIKI_BRACKET_RIGHT,   DIKS_SQUARE_BRACKET_RIGHT },
    {  STX_REMOTE_KEY_BACKSLASH,                   DIKI_BACKSLASH,       DIKS_BACKSLASH    },
    {  STX_REMOTE_KEY_SEMICOLON,                   DIKI_SEMICOLON,       DIKS_SEMICOLON    },
    {  STX_REMOTE_KEY_QUOTE,                       DIKI_QUOTE_RIGHT,     DIKS_QUOTATION    },
    {  STX_REMOTE_KEY_COMMA,                       DIKI_COMMA,           DIKS_COMMA        },
    {  STX_REMOTE_KEY_PERIOD,                      DIKI_PERIOD,          DIKS_PERIOD       },
    {  STX_REMOTE_KEY_SLASH,                       DIKI_SLASH,           DIKS_SLASH        },
    {  STX_REMOTE_KEY_BACKQUOTE,                   DIKI_QUOTE_LEFT,      DIKS_GRAVE_ACCENT },
    {  STX_REMOTE_KEY_SPACE,                       DIKI_SPACE,           DIKS_SPACE        },
    {  STX_REMOTE_KEY_TAB,                         DIKI_TAB,             DIKS_TAB          },
    {  STX_REMOTE_KEY_CAPS,                        DIKI_CAPS_LOCK,       DIKS_CAPS_LOCK    },
    {  STX_REMOTE_KEY_LSHIFT,                      DIKI_SHIFT_L,         DIKS_SHIFT        },
    {  STX_REMOTE_KEY_RSHIFT,                      DIKI_SHIFT_R,         DIKS_SHIFT        },
    {  STX_REMOTE_KEY_LCONTROL,                    DIKI_CONTROL_L,       DIKS_CONTROL      },
    {  STX_REMOTE_KEY_RCONTROL,                    DIKI_CONTROL_R,       DIKS_CONTROL      },
    {  STX_REMOTE_KEY_LMETA,                       DIKI_META_L,          DIKS_META         },
    {  STX_REMOTE_KEY_RMETA,                       DIKI_META_R,          DIKS_META         },
    {  STX_REMOTE_KEY_KBDUP,                       DIKI_UP,              DIKS_CURSOR_UP    },
    {  STX_REMOTE_KEY_KBDDOWN,                     DIKI_DOWN,            DIKS_CURSOR_DOWN  },
    {  STX_REMOTE_KEY_KBDLEFT,                     DIKI_LEFT,            DIKS_CURSOR_LEFT  },
    {  STX_REMOTE_KEY_KBDRIGHT,                    DIKI_RIGHT,           DIKS_CURSOR_RIGHT },
    {  STX_REMOTE_KEY_PAGEUP,                      DIKI_PAGE_UP,         DIKS_PAGE_UP      },
    {  STX_REMOTE_KEY_PAGEDOWN,                    DIKI_PAGE_DOWN,       DIKS_PAGE_DOWN    },
    {  STX_REMOTE_KEY_HOME,                        DIKI_HOME,            DIKS_HOME         },
    {  STX_REMOTE_KEY_END,                         DIKI_END,             DIKS_END          },
    {  STX_REMOTE_KEY_INSERT,                      DIKI_INSERT,          DIKS_INSERT       },
    {  STX_REMOTE_KEY_BACKSPACE,                   DIKI_BACKSPACE,       DIKS_BACKSPACE    },
    {  STX_REMOTE_KEY_DELETE,                      DIKI_DELETE,          DIKS_DELETE       },
    {  STX_REMOTE_KEY_KBDENTER,                    DIKI_ENTER,           DIKS_ENTER        },
    {  STX_REMOTE_KEY_F1,                          DIKI_F1,              DIKS_F1           },
    {  STX_REMOTE_KEY_F2,                          DIKI_F2,              DIKS_F2           },
    {  STX_REMOTE_KEY_F3,                          DIKI_F3,              DIKS_F3           },
    {  STX_REMOTE_KEY_F4,                          DIKI_F4,              DIKS_F4           },
    {  STX_REMOTE_KEY_F5,                          DIKI_F5,              DIKS_F5           },
    {  STX_REMOTE_KEY_F6,                          DIKI_F6,              DIKS_F6           },
    {  STX_REMOTE_KEY_F7,                          DIKI_F7,              DIKS_F7           },
    {  STX_REMOTE_KEY_F8,                          DIKI_F8,              DIKS_F8           },
    {  STX_REMOTE_KEY_F9,                          DIKI_F9,              DIKS_F9           },
    {  STX_REMOTE_KEY_F10,                         DIKI_F10,             DIKS_F10          },
    {  STX_REMOTE_KEY_F11,                         DIKI_F11,             DIKS_F11          },
    {  STX_REMOTE_KEY_F12,                         DIKI_F12,             DIKS_F12          },
    {  STX_REMOTE_KEY_F13,                         DIKI_UNKNOWN,         DIKS_CUSTOM13     },
    {  STX_REMOTE_KEY_F14,                         DIKI_UNKNOWN,         DIKS_CUSTOM14     },
    {  STX_REMOTE_KEY_F15,                         DIKI_UNKNOWN,         DIKS_CUSTOM15     },
    {  STX_REMOTE_KEY_F16,                         DIKI_UNKNOWN,         DIKS_CUSTOM16     },
    {  STX_REMOTE_KEY_ESCAPE,                      DIKI_ESCAPE,          DIKS_ESCAPE       },
    {  STX_REMOTE_KEY_PRINT_SCREEN,                DIKI_PRINT,           DIKS_PRINT        },
    {  STX_REMOTE_KEY_DVDTOPMENU,                  DIKI_UNKNOWN,         DIKS_CUSTOM17     },
    {  STX_REMOTE_KEY_DVDMENU,                     DIKI_UNKNOWN,         DIKS_CUSTOM18     },
    {  STX_REMOTE_KEY_RETURN,                      DIKI_UNKNOWN,         DIKS_CUSTOM44     },
    {  STX_REMOTE_KEY_ANGLE,                       DIKI_UNKNOWN,         DIKS_ANGLE        },
    {  STX_REMOTE_KEY_SUBTITLE,                    DIKI_UNKNOWN,         DIKS_SUBTITLE     },
    {  STX_REMOTE_KEY_DVD,                         DIKI_UNKNOWN,         DIKS_DVD          },
    {  STX_REMOTE_KEY_STOP,                        DIKI_UNKNOWN,         DIKS_STOP         },
    {  STX_REMOTE_KEY_VCRPLUS,                     DIKI_UNKNOWN,         DIKS_VCR          },
    {  STX_REMOTE_KEY_VIDEO_MODE_FIXED_480i,       DIKI_UNKNOWN,         DIKS_CUSTOM19     },
    {  STX_REMOTE_KEY_VIDEO_MODE_FIXED_480p,       DIKI_UNKNOWN,         DIKS_CUSTOM20     },
    {  STX_REMOTE_KEY_VIDEO_MODE_FIXED_720p,       DIKI_UNKNOWN,         DIKS_CUSTOM21     },
    {  STX_REMOTE_KEY_VIDEO_MODE_FIXED_1080i,      DIKI_UNKNOWN,         DIKS_CUSTOM22     },
    {  STX_REMOTE_KEY_VIDEO_MODE_HYBRID,           DIKI_UNKNOWN,         DIKS_CUSTOM23     },
    {  STX_REMOTE_KEY_VIDEO_MODE_NATIVE,           DIKI_UNKNOWN,         DIKS_CUSTOM24     },
    {  STX_REMOTE_KEY_ASPECT_CORRECTION_FULL,      DIKI_UNKNOWN,         DIKS_CUSTOM25     },
    {  STX_REMOTE_KEY_ASPECT_CORRECTION_PANEL,     DIKI_UNKNOWN,         DIKS_CUSTOM26     },
    {  STX_REMOTE_KEY_ASPECT_CORRECTION_ZOOM,      DIKI_UNKNOWN,         DIKS_CUSTOM27     },
    {  STX_REMOTE_KEY_ASPECT_CORRECTION_WIDE_ZOOM, DIKI_UNKNOWN,         DIKS_CUSTOM28     },
    {  STX_REMOTE_KEY_CC_ON,                       DIKI_UNKNOWN,         DIKS_CUSTOM29     },
    {  STX_REMOTE_KEY_CC_OFF,                      DIKI_UNKNOWN,         DIKS_CUSTOM30     },
    {  STX_REMOTE_KEY_TUNER_SWITCH,                DIKI_UNKNOWN,         DIKS_TUNER        },
    {  STX_REMOTE_KEY_VIDEO_MODE_HYBRID_720p,      DIKI_UNKNOWN,         DIKS_CUSTOM31     },
    {  STX_REMOTE_KEY_VIDEO_MODE_HYBRID_1080i,     DIKI_UNKNOWN,         DIKS_CUSTOM32     },
    {  STX_REMOTE_KEY_FPPLAY,                      DIKI_UNKNOWN,         DIKS_CUSTOM33     },
    {  STX_REMOTE_KEY_FPPAUSE,                     DIKI_UNKNOWN,         DIKS_CUSTOM34     },
    {  STX_REMOTE_KEY_FPSKIPFORWARD,               DIKI_UNKNOWN,         DIKS_CUSTOM35     },
    {  STX_REMOTE_KEY_FPSKIPREVERSE,               DIKI_UNKNOWN,         DIKS_CUSTOM36     },
    {  STX_REMOTE_KEY_FPSTOP,                      DIKI_UNKNOWN,         DIKS_CUSTOM37     },
    {  STX_REMOTE_KEY_ACTION_A,                    DIKI_UNKNOWN,         DIKS_YELLOW       },
    {  STX_REMOTE_KEY_ACTION_B,                    DIKI_UNKNOWN,         DIKS_BLUE         },
    {  STX_REMOTE_KEY_ACTION_C,                    DIKI_UNKNOWN,         DIKS_RED          },
    {  STX_REMOTE_KEY_ACTION_D,                    DIKI_UNKNOWN,         DIKS_GREEN        },
    {  STX_REMOTE_KEY_LOW_BATTERY,                 DIKI_UNKNOWN,         DIKS_CUSTOM38     },
    {  STX_REMOTE_KEY_BACK,                        DIKI_UNKNOWN,         DIKS_BACK         },
    {  STX_REMOTE_KEY_VIDEO_ON_DEMAND,             DIKI_UNKNOWN,         DIKS_CUSTOM39     },
    {  STX_REMOTE_KEY_TELETEXT,                    DIKI_UNKNOWN,         DIKS_CUSTOM40     },
    {  STX_REMOTE_KEY_ENTER_STANDBY,               DIKI_UNKNOWN,         DIKS_CUSTOM7      },
    {  STX_REMOTE_KEY_EXIT_STANDBY,                DIKI_UNKNOWN,         DIKS_CUSTOM8      },
    {  STX_REMOTE_KEY_FIND_REMOTE,                 DIKI_UNKNOWN,         DIKS_CUSTOM9      },
    {  STX_REMOTE_KEY_AUDIO,                       DIKI_UNKNOWN,         DIKS_AUDIO        },
    {  STX_REMOTE_KEY_SKIPFORWARD,                 DIKI_UNKNOWN,         DIKS_CUSTOM41     },
    {  STX_REMOTE_KEY_SKIPREVERSE,                 DIKI_UNKNOWN,         DIKS_CUSTOM42     },
    {  STX_REMOTE_KEY_SEARCH,                      DIKI_UNKNOWN,         DIKS_CUSTOM43     },
    //-----------------------------------------------------
    // sentinel
    { (U32) STX_REMOTE_KEY_UNKNOWN, DIKI_UNKNOWN, DIKS_NULL }
};


/**
 * shiftMappingsG
 */
static const ShiftMapping shiftMappingsG[] =
{
    { DIKS_GRAVE_ACCENT, DIKS_TILDE },
    { DIKS_0, DIKS_PARENTHESIS_RIGHT },
    { DIKS_1, DIKS_EXCLAMATION_MARK },
    { DIKS_2, DIKS_AT },
    { DIKS_3, DIKS_NUMBER_SIGN },
    { DIKS_4, DIKS_DOLLAR_SIGN },
    { DIKS_5, DIKS_PERCENT_SIGN },
    { DIKS_6, DIKS_CIRCUMFLEX_ACCENT },
    { DIKS_7, DIKS_AMPERSAND },
    { DIKS_8, DIKS_ASTERISK },
    { DIKS_9, DIKS_PARENTHESIS_LEFT },
    { DIKS_MINUS_SIGN, DIKS_UNDERSCORE },
    { DIKS_EQUALS_SIGN, DIKS_PLUS_SIGN },
    { DIKS_SQUARE_BRACKET_LEFT, DIKS_CURLY_BRACKET_LEFT },
    { DIKS_SQUARE_BRACKET_RIGHT, DIKS_CURLY_BRACKET_RIGHT },
    { DIKS_BACKSLASH, DIKS_VERTICAL_BAR },
    { DIKS_SEMICOLON, DIKS_COLON },
    { DIKS_APOSTROPHE, DIKS_QUOTATION },
    { DIKS_COMMA, DIKS_LESS_THAN_SIGN },
    { DIKS_PERIOD, DIKS_GREATER_THAN_SIGN },
    { DIKS_SLASH, DIKS_QUESTION_MARK },
    { (DFBInputDeviceKeySymbol) 0, (DFBInputDeviceKeySymbol) 0 }
};

/**
 * TiVoToDirectFbKeyMap
 */
static bool TiVoToDirectFbKeyMap(
          U32 tivoKeyCode,
          U16 tivoKeySymbol,
          bool isShiftSet,
          DFBInputDeviceKeyIdentifier* /* returns*/ pDfbKeyId,
          DFBInputDeviceKeySymbol* /* returns */ pDfbKeySymbol )
{
    // local aliases for convenience
    DFBInputDeviceKeyIdentifier& dfbKeyId     = *pDfbKeyId;
    DFBInputDeviceKeySymbol&     dfbKeySymbol = *pDfbKeySymbol;

    // unmapped by default
    dfbKeyId     = DIKI_UNKNOWN;
    dfbKeySymbol = DIKS_NULL;

    const TiVoToDfbKeyIdMapping* pTiVoToDfbKeyMappings = tivoToDfbKeyIdMappingsG;

    // look up the TiVo key
    for (const TiVoToDfbKeyIdMapping* p = pTiVoToDfbKeyMappings;
        p->tivoKey != (U32) STX_REMOTE_KEY_UNKNOWN; p++)
    {
        if (p->tivoKey == tivoKeyCode)
        {
            dfbKeyId     = p->dfbKeyId;
            dfbKeySymbol = p->dfbKeySymbol;
            break;
        }
    }

    // tivoKeySymbol, if non-0, should be a Unicode code point
    if (tivoKeySymbol != 0)
    {
        dfbKeySymbol = (DFBInputDeviceKeySymbol) tivoKeySymbol;
    }

    // Now apply shift to symbol if shift is pressed
    if (isShiftSet)
    {
        if ((dfbKeySymbol >= DIKS_SMALL_A) && (dfbKeySymbol <= DIKS_SMALL_Z))
        {
            dfbKeySymbol = (DFBInputDeviceKeySymbol)
                (DIKS_CAPITAL_A + (dfbKeySymbol - DIKS_SMALL_A));
        }
        else
        {
            for (const ShiftMapping *p = shiftMappingsG; p->unshifted; p++)
            {
                if (p->unshifted == dfbKeySymbol)
                {
                    dfbKeySymbol = p->shifted;
                    break;
                }
            }
        }
    }

    DBG_PRINTF(
        "%s: TiVo key %lu / sym 0x%04X  -> id 0x%04X / sym 0x%04X\n",
        __FUNCTION__,
        tivoKeyCode,
        tivoKeySymbol,
        dfbKeyId,
        dfbKeySymbol );

    if ((dfbKeyId == DIKI_UNKNOWN) &&
        (dfbKeySymbol == DIKS_NULL))
    {
        DBG_PRINTF( "%s: TiVo key %lu / sym 0x%04X not mapped, ignoring!\n",
            __FUNCTION__, tivoKeyCode, tivoKeySymbol );
        return false;
    }

    return true;
}

bool PopulateDFBInputEvent(ref<StxKeyEvent> pStxKeyEvt, DFBInputEvent* pDFBInputEvent)
{
    DFBInputDeviceKeyIdentifier dfbKeyId;
    DFBInputDeviceKeySymbol dfbKeySymbol;
    /**
     *  Following static variables are used to remember which modifier key is pressed 
     *  and add modifiers to the key which follows it.
     *  Ex: If SHIFT and an alphabet 'a' pressed, remember that SHIFT is pressed and add
     *      SHIFT modifiers to alphabet 'a' event.
     */
    static bool isShiftSet = false;
    static bool isControlSet = false;
    static bool isMetaSet = false;

    TmkMonotonicTime time;
    time.LoadCurrent();
    pDFBInputEvent->timestamp.tv_sec = (long int) (time.ToNative() >> 32);
    pDFBInputEvent->timestamp.tv_usec = (long int) (time.ToNative() & 0xFFFFFFFF);

    pDFBInputEvent->flags = (DFBInputEventFlags) (DIEF_TIMESTAMP);

    if (!(pStxKeyEvt->doNotRemap))
    {
        if (TiVoToDirectFbKeyMap( ( U32 ) pStxKeyEvt->keyCode,
                                 pStxKeyEvt->symbol,
                                 isShiftSet,
                                 /* returns */ &dfbKeyId,
                                 /* returns */ &dfbKeySymbol ))
        {

            pDFBInputEvent->flags = ( DFBInputEventFlags )
                    ( pDFBInputEvent->flags | DIEF_KEYID );
            pDFBInputEvent->key_id = dfbKeyId;

            if (dfbKeySymbol != DIKS_NULL)
            {
                pDFBInputEvent->flags = ( DFBInputEventFlags )
                    ( pDFBInputEvent->flags | DIEF_KEYSYMBOL );
                pDFBInputEvent->key_symbol = dfbKeySymbol;
            }
            else
            {
                DBG_PRINTF( "%s: KeySymbol 0x%04X not mapped \n",
                    __FUNCTION__, dfbKeySymbol );
            }
        }
        else
        {
            TvDebugAssert( "%s:TiVo To DirectFb Key Map not found \n", __FUNCTION__ );
            return false;
        }
    }
    else
    {
        pDFBInputEvent->flags = ( DFBInputEventFlags )
                ( pDFBInputEvent->flags | DIEF_KEYID );
        pDFBInputEvent->key_id = (DFBInputDeviceKeyIdentifier)pStxKeyEvt->keyCode;

        pDFBInputEvent->flags = ( DFBInputEventFlags )
            ( pDFBInputEvent->flags | DIEF_KEYSYMBOL );
        pDFBInputEvent->key_symbol = (DFBInputDeviceKeySymbol)pStxKeyEvt->symbol;
    }

    pDFBInputEvent->type = pStxKeyEvt->fKeyPress ? DIET_KEYPRESS : DIET_KEYRELEASE;

    if (pStxKeyEvt->keyCode == STX_REMOTE_KEY_LSHIFT ||
        pStxKeyEvt->keyCode == STX_REMOTE_KEY_RSHIFT ||
        isShiftSet)
    {
        pDFBInputEvent->modifiers = ( DFBInputDeviceModifierMask )
            ( pDFBInputEvent->modifiers | DIMM_SHIFT );
        pDFBInputEvent->flags = ( DFBInputEventFlags )
            ( pDFBInputEvent->flags | DIEF_MODIFIERS );

        if (!isShiftSet && pStxKeyEvt->fKeyPress)
        {
            isShiftSet = true;
        }
        else if (!pStxKeyEvt->fKeyPress && 
                 (pStxKeyEvt->keyCode == STX_REMOTE_KEY_LSHIFT ||
                  pStxKeyEvt->keyCode == STX_REMOTE_KEY_RSHIFT))
        {
            isShiftSet = false;  
        }
    }
    else if (pStxKeyEvt->keyCode == STX_REMOTE_KEY_LCONTROL ||
             pStxKeyEvt->keyCode == STX_REMOTE_KEY_RCONTROL ||
             isControlSet)
    {
        pDFBInputEvent->modifiers = ( DFBInputDeviceModifierMask )
            ( pDFBInputEvent->modifiers | DIMM_CONTROL );
        pDFBInputEvent->flags = ( DFBInputEventFlags )
            ( pDFBInputEvent->flags | DIEF_MODIFIERS );

        if (!isControlSet)
        {
            isControlSet = true;
        }
        else
        {
            isControlSet = false;  
        }
    }
    else if (pStxKeyEvt->keyCode == STX_REMOTE_KEY_LMETA ||
             pStxKeyEvt->keyCode == STX_REMOTE_KEY_RMETA ||
             isMetaSet)
    {
        pDFBInputEvent->modifiers = ( DFBInputDeviceModifierMask )
            ( pDFBInputEvent->modifiers | DIMM_META );
        pDFBInputEvent->flags = ( DFBInputEventFlags )
            ( pDFBInputEvent->flags | DIEF_MODIFIERS );

        if (!isMetaSet)
        {
            isMetaSet = true;
        }
        else
        {
            isMetaSet = false;  
        }
    }

    return true;
}
