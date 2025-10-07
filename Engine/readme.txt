Duplicate override targets (UObject-derived)

Direct subclasses of `UObject`:
- UMaterial (Public\Mesh\Material.h)
- UStaticMesh (Public\Mesh\StaticMesh.h)
- UImGuiHelper (Public\Render\UI\ImGui\ImGuiHelper.h)
- UUIWindow (Public\Render\UI\Window\UIWindow.h)
- UPathManager (Public\Manager\Path\PathManager.h)
- UResourceManager (Public\Manager\ResourceManager.h)
- UUIManager (Public\Manager\UI\UIManager.h)
- ULineBatchRenderer (Public\Render\Renderer\LineBatchRenderer.h)
- URenderer (Public\Render\Renderer\Renderer.h)
- ULevelManager (Public\Manager\Level\LevelManager.h)
- UTimeManager (Public\Manager\Time\TimeManager.h)
- UOverlayManager (Public\Manager\Overlay\OverlayManager.h)
- UInputManager (Public\Manager\Input\InputManager.h)
- ULevel (Public\Level\Level.h)
- UAxis (Public\Editor\Axis.h)
- UActorComponent (Public\Components\ActorComponent.h)
- UCamera (Public\Editor\Camera.h)
- AActor (Public\Actor\Actor.h)
- UEditor (Public\Editor\Editor.h)
- UEditorEngine (Public\Editor\EditorEngine.h)
- UWidget (Public\Render\UI\Widget\Widget.h)
- UGizmo (Public\Editor\Gizmo.h)
- UGrid (Public\Editor\Grid.h)
- UObjectPicker (Public\Editor\ObjectPicker.h)

Subclasses of `AActor` (indirectly UObject):
- AStaticMeshActor (Public\Actor\StaticMeshActor.h)

Subclasses of `UActorComponent` (indirectly UObject):
- USceneComponent (Public\Components\SceneComponent.h)
- UPrimitiveComponent (Public\Components\PrimitiveComponent.h)
- UTextComponent (Public\Components\TextComponent.h)
- UMeshComponent (Public\Components\MeshComponent.h)
- UStaticMeshComponent (Public\Components\StaticMeshComponent.h)

Subclasses of `UWidget` (indirectly UObject):
- UActorDetailWidget (Public\Render\UI\Widget\ActorDetailWidget.h)
- UConsoleWidget (Public\Render\UI\Widget\ConsoleWidget.h)
- UInputInformationWidget (Public\Render\UI\Widget\InputInformationWidget.h)
- USceneIOWidget (Public\Render\UI\Widget\SceneIOWidget.h)
- UTargetActorTransformWidget (Public\Render\UI\Widget\TargetActorTransformWidget.h)
- UViewSettingsWidget (Public\Render\UI\Widget\ViewSettingsWidget.h)
- UPIEWidget (Public\Render\UI\Widget\PIEWidget.h)
- UFPSWidget (Public\Render\UI\Widget\FPSWidget.h)
- UCameraControlWidget (Public\Render\UI\Widget\CameraControlWidget.h)
- UActorTerminationWidget (Public\Render\UI\Widget\ActorTerminationWidget.h)
- UPrimitiveSpawnWidget (Public\Render\UI\Widget\PrimitiveSpawnWidget.h)
- UActorListWidget (Public\Render\UI\Widget\ActorListWidget.h)

Subclasses of `UUIWindow` (indirectly UObject):
- UOutlinerWindow (Public\Render\UI\Window\OutlinerWindow.h)
- UDetailWindow (Public\Render\UI\Window\DetailWindow.h)
- UExperimentalFeatureWindow (Public\Render\UI\Window\ExperimentalFeatureWindow.h)
- UControlPanelWindow (Public\Render\UI\Window\ControlPanelWindow.h)
- UConsoleWindow (Public\Render\UI\Window\ConsoleWindow.h)

Notes:
- Only concrete classes that actually derive (directly or indirectly) from `UObject` are listed. Utility classes like `UClass`, `UEngineStatics`, `UDeviceResources`, and `UPipeline` do not inherit from `UObject` and are excluded.
- If `Duplicate` has a sensible default in `UObject`, you can implement overrides incrementally, starting with stateful classes (actors, components, resources, widgets/windows).