template <typename Strategy>
Node* StyledMarkupTraverser<Strategy>::Traverse(Node* start_node,
                                                Node* past_end) {
  HeapVector<Member<ContainerNode>> ancestors_to_close;
  Node* next;
  Node* last_closed = nullptr;
  for (Node* n = start_node; n && n != past_end; n = next) {
  }

  return last_closed;
}