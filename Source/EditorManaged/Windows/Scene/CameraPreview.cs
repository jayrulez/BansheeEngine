using bs;
using System;
using System.Collections;
using System.Collections.Generic;

namespace bs.Editor
{
    /** @addtogroup Scene-Editor
     *  @{
     */

    /// <summary>
    /// Shows a pinnable preview of a <see cref="Camera"/> on the scene editor window.
    /// </summary>
    internal class CameraPreview
    {
        public bool IsPinned { get; private set; } = false;
        public Camera Camera { get; }
        private Camera previewCamera;
        private SceneObject previewCameraSO;

        private GUIPanel previewPanel;

        // Render texture
        private GUIPanel renderTexturePanel;
        private GUIRenderTexture renderTextureGUI;
        private RenderTexture renderTexture;

        // Controls
        private GUIPanel controlsPanel;

        private GUILabel cameraNameLabel;
        private GUIButton pinButton;

        private string copyPath = string.Empty;

        public CameraPreview(Camera camera, GUIPanel previewsPanel)
        {
            Camera = camera;
            previewCameraSO = new SceneObject($"{Camera.SceneObject.Name}_Preview", true);
            previewCamera = previewCameraSO.AddComponent<Camera>();

            previewPanel = previewsPanel.AddPanel();

            // Render texture GUI
            renderTexturePanel = previewPanel.AddPanel();
            renderTextureGUI = new GUIRenderTexture(null);
            renderTexturePanel.AddElement(renderTextureGUI);

            // Control GUI
            controlsPanel = previewPanel.AddPanel(-1);
            GUILayoutX controlsLayout = controlsPanel.AddLayoutX();
            controlsLayout.SetHeight(16);

            cameraNameLabel = new GUILabel(string.Empty);
            pinButton = new GUIButton(string.Empty);
            pinButton.SetWidth(16);
            pinButton.SetHeight(16);
            pinButton.OnClick += () =>
            {
                IsPinned = !IsPinned;
                UpdatePinButton();
            };

            controlsLayout.AddElement(cameraNameLabel);
            controlsLayout.AddFlexibleSpace();
            controlsLayout.AddElement(pinButton);

            UpdatePinButton();
        }

        /// <summary>
        /// Sets the content of the pin button according to its current state.
        /// </summary>
        private void UpdatePinButton()
        {
            pinButton.SetContent(new GUIContent(IsPinned ? "-" : "+", IsPinned ? "Unpin" : "Pin"));
        }

        /// <summary>
        /// Shows a preview of the specified camera with the specified bounds.
        /// </summary>
        /// <param name="camera">The camera to preview</param>
        /// <param name="bounds">The bounds of the preview</param>
        public void ShowPreview(Rect2I bounds)
        {
            if (Camera.IsDestroyed)
                return;

            previewPanel.Bounds = bounds;
            cameraNameLabel.SetContent(Camera.SceneObject?.Name);

            renderTextureGUI.SetWidth(bounds.width);
            renderTextureGUI.SetHeight(bounds.height);

            if (renderTexture == null || !(renderTexture.Width == bounds.width && renderTexture.Height == bounds.height))
            {
                renderTexture = new RenderTexture(PixelFormat.RGBA8, bounds.width, bounds.height) { Priority = 1 };
                renderTextureGUI.RenderTexture = renderTexture;
            }

            previewCameraSO.Name = $"{Camera.SceneObject.Name}_Preview";
            previewCameraSO.Position = Camera.SceneObject.Position;
            previewCameraSO.Rotation = Camera.SceneObject.Rotation;

            CopyCamera(Camera, previewCamera);

            if (previewCamera.Viewport.Target != renderTexture)
                previewCamera.Viewport.Target = renderTexture;
        }

        private void CopyCamera(Camera sourceCamera, Camera targetCamera)
        {
            // This implementation will be replaced by a call to CopyCameraProperties when the implementation is done.
            if (targetCamera.AspectRatio != sourceCamera.AspectRatio)
                targetCamera.AspectRatio = sourceCamera.AspectRatio;

            if (targetCamera.FarClipPlane != sourceCamera.FarClipPlane)
                targetCamera.FarClipPlane = sourceCamera.FarClipPlane;

            if (targetCamera.FieldOfView != sourceCamera.FieldOfView)
                targetCamera.FieldOfView = sourceCamera.FieldOfView;

            if (targetCamera.Layers != sourceCamera.Layers)
                targetCamera.Layers = sourceCamera.Layers;

            if (targetCamera.NearClipPlane != sourceCamera.NearClipPlane)
                targetCamera.NearClipPlane = sourceCamera.NearClipPlane;

            if (targetCamera.OrthoHeight != sourceCamera.OrthoHeight)
                targetCamera.OrthoHeight = sourceCamera.OrthoHeight;

            if (targetCamera.OrthoWidth != sourceCamera.OrthoWidth)
                targetCamera.OrthoWidth = sourceCamera.OrthoWidth;

            if (targetCamera.Priority != sourceCamera.Priority)
                targetCamera.Priority = sourceCamera.Priority;

            if (targetCamera.ProjectionType != sourceCamera.ProjectionType)
                targetCamera.ProjectionType = sourceCamera.ProjectionType;

            if (targetCamera.SampleCount != sourceCamera.SampleCount)
                targetCamera.SampleCount = sourceCamera.SampleCount;

            targetCamera.RenderSettings = sourceCamera.RenderSettings;

            if (targetCamera.Viewport.Area != sourceCamera.Viewport.Area)
                targetCamera.Viewport.Area = sourceCamera.Viewport.Area;

            if (targetCamera.Viewport.ClearColor != sourceCamera.Viewport.ClearColor)
                targetCamera.Viewport.ClearColor = sourceCamera.Viewport.ClearColor;

            if (targetCamera.Viewport.ClearDepth != sourceCamera.Viewport.ClearDepth)
                targetCamera.Viewport.ClearDepth = sourceCamera.Viewport.ClearDepth;

            if (targetCamera.Viewport.ClearFlags != sourceCamera.Viewport.ClearFlags)
                targetCamera.Viewport.ClearFlags = sourceCamera.Viewport.ClearFlags;

            if (targetCamera.Viewport.ClearStencil != sourceCamera.Viewport.ClearStencil)
                targetCamera.Viewport.ClearStencil = sourceCamera.Viewport.ClearStencil;
        }

        private void CopyCameraProperties(Camera sourceCamera, Camera targetCamera)
        {
            var sourceCameraObj = new SerializableObject(sourceCamera);

            var targetCameraObj = new SerializableObject(targetCamera);

            CopyProperties(sourceCameraObj, targetCameraObj);
        }

        private void CopyProperties(SerializableObject source, SerializableObject target)
        {
            for (int i = 0; i < source.Fields.Length; i++)
            {
                var sourceProperty = source.Fields[i].GetProperty();
                var targetProperty = target.Fields[i].GetProperty();

                CopyProperty(sourceProperty, targetProperty);
            }
        }

        private void CopyProperty(SerializableProperty source, SerializableProperty target)
        {
            switch (source.Type)
            {
                case SerializableProperty.FieldType.Int:
                    {
                        CopyPropertyValue<int>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Float:
                    {
                        CopyPropertyValue<float>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Bool:
                    {
                        CopyPropertyValue<bool>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.String:
                    {
                        CopyPropertyValue<string>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Color:
                    {
                        CopyPropertyValue<Color>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Vector2:
                    {
                        CopyPropertyValue<Vector2>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Vector3:
                    {
                        CopyPropertyValue<Vector3>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Vector4:
                    {
                        CopyPropertyValue<Vector4>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.GameObjectRef:
                    {

                    }

                    break;

                case SerializableProperty.FieldType.Resource:
                    {

                    }

                    break;

                case SerializableProperty.FieldType.Object:
                    {
                        CopyProperties(source.GetObject(), target.GetObject());
                    }
                    break;

                case SerializableProperty.FieldType.Array:
                    {
                        target.SetValue(source.GetArray());
                        break;
                    }

                case SerializableProperty.FieldType.List:
                    for (int i = 0; i < source.GetList().GetLength(); i++)
                        CopyProperty(source.GetList().GetProperty(i), target.GetList().GetProperty(i));
                    break;

                case SerializableProperty.FieldType.Dictionary:
                    var sourceDictionary = source.GetValue<IDictionary>();
                    var targetDictionary = target.GetValue<IDictionary>();
                    foreach (var key in sourceDictionary.Keys)
                    {
                        if (targetDictionary.Contains(key))
                        {
                            targetDictionary[key] = sourceDictionary[key];
                        }
                        else
                        {
                            targetDictionary.Add(key, sourceDictionary[key]);
                        }
                    }
                    target.SetValue(targetDictionary);
                    break;

                case SerializableProperty.FieldType.RRef:
                    { }
                    break;

                case SerializableProperty.FieldType.ColorGradient:
                    { }
                    break;

                case SerializableProperty.FieldType.Curve:
                    { }
                    break;

                case SerializableProperty.FieldType.FloatDistribution:
                    { }
                    break;

                case SerializableProperty.FieldType.ColorDistribution:
                    { }
                    break;

                case SerializableProperty.FieldType.Quaternion:
                    {
                        CopyPropertyValue<Quaternion>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Enum:
                    {
                        CopyPropertyValue<int>(source, target);
                    }
                    break;

                case SerializableProperty.FieldType.Vector2Distribution:
                    { }
                    break;

                case SerializableProperty.FieldType.Vector3Distribution:
                    { }
                    break;

                case SerializableProperty.FieldType.ColorGradientHDR:
                    { }
                    break;

                default:
                    Debug.LogError($"CopyProperty for '{source.Type}' is not implemented.");
                    break;
            }
        }

        private void CopyPropertyValue<T>(SerializableProperty source, SerializableProperty target)
        {
            if (source.IsValueType)
            {
                try
                {
                    var sourceValue = source.GetValue<T>();

                    var targetValue = target.GetValue<T>();

                    if (!sourceValue.Equals(targetValue))
                    {
                        target.SetValue(sourceValue);
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception(ex.StackTrace);
                }
            }
            else
            {
                Debug.LogWarning($"CopyPropertyValue called for '{source.Type}' which is not a value type.");
            }
        }

        /// <summary>
        /// Destroys the GUI elements representing the preview box.
        /// </summary>
        public void Destroy()
        {
            IsPinned = false;
            previewCameraSO.Destroy();
            previewPanel?.Destroy();
        }
    }

    /** @} */
}
