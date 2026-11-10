add_library(at_ui STATIC
    Document.cpp Theme.cpp CompSettingsDialog.cpp ViewerPanel.cpp ProjectPanel.cpp
    PropertiesPanel.cpp TimelinePanel.cpp MainWindow.cpp)
target_link_libraries(at_ui PUBLIC at_media at_comp at_project Qt6::Widgets)
target_compile_definitions(at_ui PRIVATE AT_VERSION=\"${PROJECT_VERSION}\")
