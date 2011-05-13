bin_PROGRAMS = search yappo_makeindex yappo_margepos yappod_front yappod_core
include_HEADERS=*.h

YAPPO_BASE_OBJS    = yappo_alloc.c yappo_db.c yappo_index.c yappo_index_pos.c yappo_index_filedata.c yappo_index_deletefile.c
YAPPO_SEARCH_OBJS  = yappo_search.c yappo_ngram.c yappo_linklist.c


search_SOURCES  = $(YAPPO_BASE_OBJS) $(YAPPO_SEARCH_OBJS) search.c

yappo_makeindex_SOURCES  = $(YAPPO_BASE_OBJS) yappo_ngram.c yappo_minibtree.c yappo_makeindex.c
yappo_margepos_SOURCES   = $(YAPPO_BASE_OBJS) $(YAPPO_SEARCH_OBJS) yappo_margepos.c

yappod_front_SOURCES  = $(YAPPO_BASE_OBJS) $(YAPPO_SEARCH_OBJS) yappod_front.c
yappod_core_SOURCES   = $(YAPPO_BASE_OBJS) $(YAPPO_SEARCH_OBJS) yappod_core.c
