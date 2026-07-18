Qt = import( 'qt6' )

QtDeps = dependency(
	'qt6',
	modules: [ 'Core', 'Gui', 'Widgets' ],
	private_headers: true,
)

CoreMocs = Qt.compile_moc(
	headers : [ 'decorator.hpp' ],
	sources : [ 'main.cpp' ],
 	dependencies: QtDeps,
)

qt6deco = executable( 'wf-qt6-decorator',
    ['main.cpp', 'protocol.cpp', CoreMocs],
    install: true,
    dependencies: [QtDeps, wayland_client, wf_client_protos]
)
