{ 
  'targets': [ 
    { 
      'target_name': 'NodeQtSql', 
      'type': 'static_library',
      'sources': [ 
        'src/bindings/nodequery.cpp', 
        'src/db/connection.cpp', 
        'src/db/connection_sources.cpp', 
        'src/db/oledbconnectionparser.cpp', 
        'src/nodeqtsql.cpp', 
		    'src/exception.cpp'
      ], 
      "defines": [ "_UNICODE", "UNICODE", "NODEQTSQL_LIBRARY", "ELPP_NO_DEFAULT_LOG_FILE" ], 
      'include_dirs': [
        'src/', 
        '../../src/', 
        '../v8/include/',
        'deps/',
        'deps/qt5/include',
        'deps/qt5/include/QtCore',
        'deps/qt5/include/QtSql'
      ]
    }
  ] 
}