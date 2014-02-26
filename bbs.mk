
include settings.mk

OBJDIR := bbsobj/
CFLAGS := -g -Wall -Isrc -Isrc/netlink/include

TARGET := bbs
DIRS := src/utils src/netlink/src src/bbs
OBJS := bbstest.o

WIN_CFLAGS := $(WIN_CFLAGS) -static
WIN_LIBS := -lws2_32
WIN_LDFLAGS := -static
WIN_CC := gcc
WIN_CXX := g++

all : start_rule

run :
	./bbstest

include Makefile.inc
