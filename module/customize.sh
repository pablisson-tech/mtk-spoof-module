SKIPUNZIP=1

ui_print "- Extraindo arquivos..."
unzip -o "$ZIPFILE" 'module.prop' -d $MODPATH >&2
unzip -o "$ZIPFILE" 'zygisk/*' -d $MODPATH >&2

# Configurar permissões
set_perm_recursive $MODPATH 0 0 0755 0644

ui_print "- Módulo MTK Spoof instalado!"
ui_print "- Reinicie o aparelho para aplicar as alterações."
