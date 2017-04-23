
#if 0
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}
#else
#include <string.h>
#include <stdlib.h>
char *dtostrf(double val, int width, unsigned int prec, char *sout)
{
  int decpt, sign, reqd, pad;
  const char *s, *e;
  char *p;
  s = fcvt(val, prec, &decpt, &sign);
  if (prec == 0 && decpt == 0) {
  s = (*s < '5') ? "0" : "1";
    reqd = 1;
  } else {
    reqd = strlen(s);
    if (reqd > decpt) reqd++;
    if (decpt == 0) reqd++;
  }
  if (sign) reqd++;
  p = sout;
  e = p + reqd;
  pad = width - reqd;
  if (pad > 0) {
    e += pad;
    while (pad-- > 0) *p++ = ' ';
  }
  if (sign) *p++ = '-';
  if (decpt <= 0 && prec > 0) {
    *p++ = '0';
    *p++ = '.';
    e++;
    while ( decpt < 0 ) {
      decpt++;
      *p++ = '0';
    }
  }   
  while (p < e) {
    *p++ = *s++;
    if (p == e) break;
    if (--decpt == 0) *p++ = '.';
  }
  if (width < 0) {
    pad = (reqd + width) * -1;
    while (pad-- > 0) *p++ = ' ';
  }
  *p = 0;
  return sout;
}
#endif
