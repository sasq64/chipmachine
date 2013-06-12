
include settings.mk

OBJDIR := obj/
CFLAGS := $(CFLAGS) -g -Wall -I.

TARGET := words
OBJS := WordDAC.o utils.o

WIN_CC := gcc
WIN_CXX := g++

LINUX_CFLAGS := $(LINUX_CFLAGS)

include Makefile.inc