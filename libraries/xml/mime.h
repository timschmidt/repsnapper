// mime.h for TurboIRC

/*
 * MimeCodes.h - MIME Encoding and decoding filters, using STL-like iterators.
 */

#ifndef MIME_CODES_H
#define MIME_CODES_H


/******************************************************************************
 * MimeCoder -  Abstract base class for MIME filters.
 ******************************************************************************/
template <class InIter, class OutIter>
class MimeCoder
{
public:
	 virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd ) = 0;
    virtual OutIter Finish( OutIter out ) = 0;
};

/******************************************************************************
 * Base64
 ******************************************************************************/
template <class InIter, class OutIter>
class Base64Encoder : public MimeCoder<InIter, OutIter>
{
public:
    Base64Encoder() : its3Len(0), itsLinePos(0) {}
    virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
	 virtual OutIter Finish( OutIter out );
private:
    int             itsLinePos;
	 unsigned char   itsCurr3[3];
    int             its3Len;
    void EncodeCurr3( OutIter& out );
};

template <class InIter, class OutIter>
class Base64Decoder : public MimeCoder<InIter, OutIter>
{
public:
    Base64Decoder() : its4Len(0), itsEnded(0) {}
    virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
	 virtual OutIter Finish( OutIter out );
private:
    int             itsEnded;
    unsigned char   itsCurr4[4];
    int             its4Len;
	 int             itsErrNum;
    void DecodeCurr4( OutIter& out );
};

/******************************************************************************
 * Quoted-Printable
 ******************************************************************************/
template <class InIter, class OutIter>
class QpEncoder : public MimeCoder<InIter, OutIter>
{
public:
    QpEncoder() : itsLinePos(0), itsPrevCh('x') {}
    virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
    virtual OutIter Finish( OutIter out );
private:
    int             itsLinePos;
	 unsigned char   itsPrevCh;
};

template <class InIter, class OutIter>
class QpDecoder : public MimeCoder<InIter, OutIter>
{
public:
	 QpDecoder() : itsHexLen(0) {}
	 virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
    virtual OutIter Finish( OutIter out );
private:
    int             itsHexLen;
    unsigned char   itsHex[2];
};


#endif // MIME_CODES_H


#define TEST_MIME_CODES

#define ___

static const int cLineLen = 72;

/******************************************************************************
 * Base64Encoder
 ******************************************************************************/
static const char cBase64Codes[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char CR = 13;
static const char LF = 10;

template <class InIter, class OutIter>
OutIter Base64Encoder<InIter, OutIter>::Filter(
	 OutIter out,
    InIter inBeg,
    InIter inEnd )
{
    for(;;) {
        for(; itsLinePos < cLineLen; itsLinePos += 4) {
            for (; its3Len < 3; its3Len++) {
                if (inBeg == inEnd)
___ ___ ___ ___ ___ return out;
                itsCurr3[its3Len] = *inBeg;
                ++inBeg;
            }
            EncodeCurr3(out);
            its3Len = 0;
		  } // for loop until end of line
        *out++ = CR;
        *out++ = LF;
        itsLinePos = 0;
    } // for (;;)
//    return out;
}

template <class InIter, class OutIter>
OutIter Base64Encoder<InIter, OutIter>::Finish( OutIter out )
{
    if (its3Len)
        EncodeCurr3(out);
	 its3Len = 0;
    itsLinePos = 0;

    return out;
}

template <class InIter, class OutIter>
void Base64Encoder<InIter, OutIter>::EncodeCurr3( OutIter& out )
{
    if (its3Len < 3) itsCurr3[its3Len] = 0;

    *out++ = cBase64Codes[ itsCurr3[0] >> 2 ];
    *out++ = cBase64Codes[ ((itsCurr3[0] & 0x3)<< 4) |
                           ((itsCurr3[1] & 0xF0) >> 4) ];
	 if (its3Len == 1) *out++ = '=';
    else
        *out++ = cBase64Codes[ ((itsCurr3[1] & 0xF) << 2) |
                               ((itsCurr3[2] & 0xC0) >>6) ];
    if (its3Len < 3) *out++ = '=';
    else
        *out++ = cBase64Codes[ itsCurr3[2] & 0x3F ];
}

/******************************************************************************
 * Base64Decoder
 ******************************************************************************/
#define XX 127

static const unsigned char cIndex64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	 XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

template <class InIter, class OutIter>
OutIter Base64Decoder<InIter, OutIter>::Filter(
    OutIter out,
    InIter inBeg, 
    InIter inEnd )
{
    unsigned char c;
    itsErrNum = 0;

	 for (;;) {
        while (its4Len < 4) {
            if (inBeg == inEnd)
___ ___ ___ ___ return out;
            c = *inBeg;
            if ((cIndex64[c] != XX) || (c == '='))
                itsCurr4[its4Len++] = c;
            else if ((c != CR) && (c != LF)) ++itsErrNum; // error
            ++inBeg;
        } // while (its4Len < 4)
        DecodeCurr4(out);
        its4Len = 0;
    } // for (;;)
    
//	 return out;
}

template <class InIter, class OutIter>
OutIter Base64Decoder<InIter, OutIter>::Finish( OutIter out )
{
    its4Len = 0;
    if (itsEnded) return out;
    else { // error
        itsEnded = 0;
        return out;
    }
}

template <class InIter, class OutIter>
void Base64Decoder<InIter, OutIter>::DecodeCurr4( OutIter& out )
{
    if (itsEnded) {
        ++itsErrNum;
        itsEnded = 0;
    }

    for (int i = 0; i < 2; i++)
        if (itsCurr4[i] == '=') {
            ++itsErrNum; // error
___ ___ ___ return;
        }
        else itsCurr4[i] = cIndex64[itsCurr4[i]];

    *out++ = (itsCurr4[0] << 2) | ((itsCurr4[1] & 0x30) >> 4);
    if (itsCurr4[2] == '=') {
        if (itsCurr4[3] == '=') itsEnded = 1;
        else ++itsErrNum;
    } else {
        itsCurr4[2] = cIndex64[itsCurr4[2]];
        *out++ = ((itsCurr4[1] & 0x0F) << 4) | ((itsCurr4[2] & 0x3C) >> 2);
        if (itsCurr4[3] == '=') itsEnded = 1;
        else *out++ = ((itsCurr4[2] & 0x03) << 6) | cIndex64[itsCurr4[3]];
    }
}

/******************************************************************************
 * QpEncoder
 ******************************************************************************/

static const char cBasisHex[] = "0123456789ABCDEF";

template <class InIter, class OutIter>
OutIter QpEncoder<InIter, OutIter>::Filter(
    OutIter out,
    InIter inBeg, 
    InIter inEnd )
{
   unsigned char c;
    
    for (; inBeg != inEnd; ++inBeg) {
		  c = *inBeg;

        // line-breaks
        if (c == '\n') {
            if (itsPrevCh == ' ' || itsPrevCh == '\t') {
                *out++ = '='; // soft & hard lines
                *out++ = c;
            }
            *out++ = c;
            itsLinePos = 0;
            itsPrevCh = c;
        }

        // non-printable
		  else if ( (c < 32 && c != '\t')
                  || (c == '=')
                  || (c >= 127)
                  // Following line is to avoid single periods alone on lines,
                  // which messes up some dumb SMTP implementations, sigh...
                  || (itsLinePos == 0 && c == '.') ) {
            *out++ = '=';
            *out++ = cBasisHex[c >> 4];
            *out++ = cBasisHex[c & 0xF];
            itsLinePos += 3;
            itsPrevCh = 'A'; // close enough
        }
        
        // printable characters
		  else {
            *out++ = itsPrevCh = c;
            ++itsLinePos;
        }

        if (itsLinePos > cLineLen) {
            *out++ = '=';
            *out++ = itsPrevCh = '\n';
            itsLinePos = 0;
        }
    } // for loop over all input
    
    return out;
}

template <class InIter, class OutIter>
OutIter QpEncoder<InIter, OutIter>::Finish( OutIter out )
{
    if (itsLinePos) {
        *out++ = '=';
        *out++ = '\n';
    }

    itsLinePos = 0;
    itsPrevCh = 'x';

    return out;
}

/******************************************************************************
 * QpDecoder
 ******************************************************************************/

static const unsigned char cIndexHex[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	 XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

template <class InIter, class OutIter>
OutIter QpDecoder<InIter, OutIter>::Filter(
    OutIter out,
    InIter inBeg, 
	 InIter inEnd )
{
    unsigned char c, c1, c2;
    int errn = 0;

    for (; inBeg != inEnd; ++inBeg) {
        if (itsHexLen) {                        // middle of a Hex triplet
            if (*inBeg == '\n') itsHexLen = 0;      // soft line-break
            else {                                  // Hex code
                itsHex[itsHexLen-1] = *inBeg;
                if (itsHexLen++ == 2) {
                    if (XX == (c1 = cIndexHex[itsHex[0]])) ++errn;
                    if (XX == (c2 = cIndexHex[itsHex[1]])) ++errn;
                    c = (c1 << 4) | c2;
						  if (c != '\r') *out++ = c;
                    itsHexLen = 0;
                }
            }
        }
        else if (*inBeg == '=') itsHexLen = 1;  // beginning of a new Hex triplet
        else *out++ = *inBeg;                   // printable character
    }
    
    return out;
}

template <class InIter, class OutIter>
OutIter QpDecoder<InIter, OutIter>::Finish( OutIter out )
{
    if (itsHexLen) { // error
        itsHexLen = 0;
___ ___ return out;
    }
    return out;
}

#undef XX

/*
#ifdef TEST_MIME_CODES

#include <iostream.h>
#include <string.h>

int main(int,char**)
{
	 char c[256];
	 char m[256];
	 char* o;
	 MimeCoder<char*, char*> *e, *d;

	 cout << "Used base64 [b] or quoted-printable [q] ?\n";
	 cin >> c;
	 if (c[0] == 'b' || c[0] == 'B') {
		  e = new Base64Encoder<char*, char*>;
		  d = new Base64Decoder<char*, char*>;
	 }
	 else {
		  e = new QpEncoder<char*, char*>;
		  d = new QpDecoder<char*, char*>;
	 }

	 cout << "Enter some text to encode.\n";
	 cin >> c;

	 cout << "\n\nThe encoded text:\n";
	 o = e->Filter(m, c, c + strlen(c));
	 o = e->Finish(o);
	 *o = '\0';
	 cout << m << flush;

	 cout << "\n\nDecoded:\n";
	 o = d->Filter(c, m, m + strlen(m));
	 o = d->Finish(o);
	 *o = '\0';
	 cout << c << flush;

	 delete e;
	 delete d;

	 return 0;
}

#endif // TEST_MIME_CODES
*/
