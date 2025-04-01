int main(){
    bool preserve_newlines = new_style.ShouldPreserveBreaks();
    if (preserve_newlines && is_text_combine_) [[unlikely]] {
      preserve_newlines = false;
    }
}