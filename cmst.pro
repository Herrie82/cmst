#  Need a make file to make other make files
TEMPLATE = subdirs
SUBDIRS = ./apps/cmstapp ./apps/rootapp

# cmst build variables
include(cmst.pri)

# translations
TRANSLATIONS += ./translations/cmst_en_US.ts
TRANSLATIONS += ./translations/cmst_ru_RU.ts
TRANSLATIONS += ./translations/cmst_zh_CN.ts
TRANSLATIONS += ./translations/cmst_de_DE.ts
TRANSLATIONS += ./translations/cmst_pl_PL.ts
TRANSLATIONS += ./translations/cmst_it_IT.ts
TRANSLATIONS += ./translations/cmst_es_ES.ts
TRANSLATIONS += ./translations/cmst_es_CO.ts
