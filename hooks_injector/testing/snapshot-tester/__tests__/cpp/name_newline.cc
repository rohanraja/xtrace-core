PaintLayerScrollableArea::FreezeScrollbarsRootScope::
    ~FreezeScrollbarsRootScope() {
  if (scrollable_area_)
    scrollable_area_->ClearScrollbarRoot();
}