#ifndef _DMODELVIEWERMODEL_H
#define _DMODELVIEWERMODEL_H

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMutex>
#include <Qt3DRender>

class CameraUtils
{
public:
    struct CameraPosition {
        QVector3D position;
        QVector3D target;
        float distance;
    };

    struct BoundingSphere {
        QVector3D center;
        float radius;
    };

    static float calculateDistanceToFit(float objectRadius,
                                        float fovDegrees,
                                        float viewportWidth,
                                        float viewportHeight);

    static BoundingSphere calculateBoundingSphere(const QVector<QVector3D>& vertices);

    static float getAutoAdjustCamera(QVector<QVector3D>& vertices,float verticalAngle,int viewportwidth,int viewportheight);
};

class Mesh
{
public:
    Mesh();
    void destroy();

    // 获取模型包围盒
    void getBoundingBox(QVector3D &min, QVector3D &max) const;
    // 获取模型中心点和尺寸
    QVector3D getBoundingBoxCenter() const;
    QVector3D getBoundingBoxSize() const;

    void rotateVertices(const QMatrix4x4 &rotationMatrix);

    void normalizeModel();

    void computeNormals();

    static bool loadOBJ(const QString& filePath, Mesh& mesh);

public:
    QVector<QVector3D> vertices;
    QVector<QVector2D> texCoords;
    QVector<GLuint> indices;
    QOpenGLTexture *texture;
    QVector<QVector3D> normals;

public:
    QOpenGLVertexArrayObject& VAO();
    QOpenGLBuffer& VBO();
    QOpenGLBuffer& TexVbo();
    QOpenGLBuffer& IBO();
    QOpenGLBuffer& NORMAVBO();
    void setupMeshToGraphics(QOpenGLShaderProgram* shader);
private:
    // 顶点缓冲对象
    QOpenGLBuffer _vbo;
    // 纹理顶点缓存对象
    QOpenGLBuffer _texVbo;
    // 绘制顶点索引对象
    QOpenGLBuffer _ibo;
    // 顶点数组对象
    QOpenGLVertexArrayObject _vao;
    // 法线信息
    QOpenGLBuffer _normalvbo;
};

class OpenGLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{

public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    void render() override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
    void synchronize(QQuickFramebufferObject *item) override;

public:
    void setFilePath(QString filepath);
    void initalModel();

private:
    void initializeGL();
    void setupShaderProgram();
    void renderScene();
    void renderMesh();

    QOpenGLShaderProgram *m_shaderProgram;
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model;

    float m_rotationAngle;
    QSize m_viewportSize;

    Mesh *m_mesh;

private:
    mutable QMutex m_mutex;
    QString m_filepath;
    QString m_pendingFilePath;
    bool model_active;
};

class OpenGLModelRenderItem : public QQuickFramebufferObject
{
    Q_OBJECT

public:
    OpenGLModelRenderItem(QQuickItem *parent = nullptr);
    QQuickFramebufferObject::Renderer *createRenderer() const override;

protected:
    void synchronizeRenderer(QQuickFramebufferObject::Renderer *renderer);

public slots:
    void setFilePath(QString filepath);

private:
    mutable OpenGLRenderer* render;
    QString m_filepath;
};

#endif
