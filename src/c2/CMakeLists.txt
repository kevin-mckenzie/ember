add_custom_target(
  "install-python"
  COMMAND pyinstaller --onefile --strip --noconsole --clean --distpath
          ${CMAKE_SOURCE_DIR}/dist/ --name c2 ${CMAKE_CURRENT_LIST_DIR}/c2.py
  COMMAND staticx ${CMAKE_SOURCE_DIR}/dist/c2 ${CMAKE_SOURCE_DIR}/dist/c2)
