
void TestMethodWithConst(){
  const SelectionState& selection_state = GetSelectionStateFor(layout_text);

  const unsigned start_in_block2 = paint_range_->start_offset.value();
  int y=34;
  y = y+ 1;
  if(true){
    int x = 23;
    const unsigned start_in_block = paint_range_->start_offset.value();
    y = y+ 1;
  }
  
}