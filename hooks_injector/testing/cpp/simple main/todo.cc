void forStatementVarInit(){

  for (const String& format_name : format_names) {
    if (ClipboardItem::supports(format_name)) {
      clipboard_item_data_.emplace_back(format_name,
                                        /* Placeholder value. */ nullptr);
    }
  }
}

void PointerDeclar(){
    int *a = new int(5);
}