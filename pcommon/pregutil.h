/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
#ifndef __PREGUTIL_H
#define __PREGUTIL_H
/*******************************************************************************
 FILE         :   pregutil.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Some useful regular expressions.

 CREATION DATE:   14 Apr 1998
*******************************************************************************/
#ifndef __PCOMMON_H
#include <pcommon.h>
#endif /* PCOMMON.H */

#define RSUB(n)      "("n")"                          // ������������ � �������

#define RCCLASS(n)   "["n"]"                          // ����� ��������

#define RDECDIGIT    "[0-9]"                          // ���������� �����
#define ROCTDIGIT    "[0-7]"                          // ������������ �����
#define RHEXDIGIT    "[0-9A-Fa-f]"                    // ����������������� �����
#define RDECNOZERO   "[1-9]"                          // ���������� ����� ��� ����
#define ROCTNOZERO   "[1-7]"                          // ������������ ����� ��� ����
#define RHEXNOZERO   "[1-9A-Fa-f]"                    // ����������������� ����� ��� ����
#define RDECNUM      "0|"RDECNOZERO RDECDIGIT"*"      // ���������� ����� ��� �����
#define RHEXNUM      "0[Xx]"RHEXDIGIT"+"              // ����������������� ����� ��� �����
#define ROCTNUM      "0"ROCTDIGIT"*"                  // ������������ ����� ��� �����
#define RINTEGER     RDECNUM "|" RHEXNUM "|" ROCTNUM  // ����� ����� � ����� ����� (dec/oct/hex)
#define RSTRIPL      "[^ ].*"                         // ������� ���������� �������
#define RSTRIPR      ".*[^ ]"                         // ������� ��������� �������
#define RSTRIP       "[^ ].*[^ ]|[^ ]"                // ������� ������� ����� � ������


#ifndef PCOMN_PL_AS400   // �� ������ ������ ����������� EBCDIC-���������

#  define RSEQALPHA    "A-Za-z"                         // ��������� - ��� �����
#  define RSEQALNUM    "0-9A-Za-z"                      // ��������� - ��� ����� � �����
#  define RSEQUPPER    "A-Z"                            // ��������� - ������� �������
#  define RSEQLOWER    "a-z"                            // ��������� - ������ �������

#else // AS400/EBCDIC

#  define RSEQALPHA    "A-IJ-RS-Za-ij-rs-z"             // ��������� - ��� �����
#  define RSEQALNUM    "0-9A-IJ-RS-Za-ij-rs-z"          // ��������� - ��� ����� � �����
#  define RSEQUPPER    "A-IJ-RS-Z"                      // ��������� - ������� �������
#  define RSEQLOWER    "a-ij-rs-z"                      // ��������� - ������ �������

#endif

#define RALPHA  RCCLASS(RSEQALPHA)                       // ��������� - ��� �����
#define RALNUM  RCCLASS(RSEQALNUM)                       // ��������� - ��� ����� � �����
#define RUPPER  RCCLASS(RSEQUPPER)                       // ��������� - ������� �������
#define RLOWER  RCCLASS(RSEQLOWER)                       // ��������� - ������ �������

#define RCNAME  RCCLASS(RSEQALPHA"_")RCCLASS(RSEQALNUM"_")"*"  // ��� (� ����� ����� ����������������)

#endif /* __PREGUTIL_H */
