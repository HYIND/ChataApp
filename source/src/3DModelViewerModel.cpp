#include "3DModelViewerModel.h"
#include <QOpenGLFramebufferObject>
#include <QTimer>
#include <QQuickWindow>

float CameraUtils::calculateDistanceToFit(float objectRadius, float fovDegrees, float viewportWidth, float viewportHeight)
{
    // 确保输入有效
    if (objectRadius <= 0.0f) {
        return 5.0f; // 默认距离
    }

    if (viewportHeight <= 0.0f) {
        viewportHeight = 1.0f;
    }

    // 将视野角度转换为弧度
    float fovRadians = qDegreesToRadians(fovDegrees);

    // 计算宽高比
    float aspectRatio = viewportWidth / viewportHeight;

    // 计算垂直和水平视野
    float fovVertical = fovRadians;
    float fovHorizontal = 2.0f * std::atan(std::tan(fovVertical / 2.0f) * aspectRatio);

    // 计算基于视野的最小距离
    float distanceVertical = objectRadius / std::tan(fovVertical / 2.0f);
    float distanceHorizontal = objectRadius / std::tan(fovHorizontal / 2.0f);

    // 取较大值确保物体完全可见
    float minDistance = std::max(distanceVertical, distanceHorizontal);

    // 添加安全边距（20%）
    float margin = 0.2f;
    float safeDistance = minDistance * (1.0f + margin);

    // 确保最小距离
    const float MIN_DISTANCE = 0.1f;
    if (safeDistance < MIN_DISTANCE) {
        safeDistance = MIN_DISTANCE;
    }

    // 确保最大距离（避免过远）
    const float MAX_DISTANCE = 1000.0f;
    if (safeDistance > MAX_DISTANCE) {
        safeDistance = MAX_DISTANCE;
    }

    return safeDistance;
}

float CameraUtils::getAutoAdjustCamera(QVector<QVector3D> &vertices, float verticalAngle, int viewportwidth, int viewportheight)
{
    CameraUtils::BoundingSphere sphere = CameraUtils::calculateBoundingSphere(vertices);

    if (sphere.radius <= 0.0f) {
        sphere.radius = 1.0f; // 默认半径
    }

    // 计算最佳距离
    float optimalDistance = CameraUtils::calculateDistanceToFit(
        sphere.radius,
        verticalAngle, // 视野角度
        viewportwidth,
        viewportheight
        );

    return optimalDistance;
}

CameraUtils::BoundingSphere CameraUtils::calculateBoundingSphere(const QVector<QVector3D>& vertices) {
    if (vertices.isEmpty()) {
        return {QVector3D(0, 0, 0), 1.0f};
    }

    // 找到边界框
    QVector3D minPoint = vertices[0];
    QVector3D maxPoint = vertices[0];

    for (const auto& vertex : vertices) {
        minPoint.setX(std::min(minPoint.x(), vertex.x()));
        minPoint.setY(std::min(minPoint.y(), vertex.y()));
        minPoint.setZ(std::min(minPoint.z(), vertex.z()));

        maxPoint.setX(std::max(maxPoint.x(), vertex.x()));
        maxPoint.setY(std::max(maxPoint.y(), vertex.y()));
        maxPoint.setZ(std::max(maxPoint.z(), vertex.z()));
    }

    // 计算中心和半径
    QVector3D center = (minPoint + maxPoint) * 0.5f;
    float radius = 0.0f;

    for (const auto& vertex : vertices) {
        float distance = (vertex - center).length();
        radius = std::max(radius, distance);
    }

    return {center, radius};
}

Mesh::Mesh() : texture(nullptr), _ibo(QOpenGLBuffer::IndexBuffer)
{
}

void Mesh::destroy()
{
    if (texture) {
        texture->destroy();
        delete texture;
        texture = nullptr;
    }
}

void Mesh::getBoundingBox(QVector3D &min, QVector3D &max) const
{
    if (vertices.empty()) {
        min = max = QVector3D(0, 0, 0);
        return;
    }

    // 初始化min和max为第一个顶点的位置
    min = max = vertices[0];

    // 遍历所有顶点，找出最小和最大坐标
    for (const QVector3D &pos : vertices) {

        min.setX(qMin(min.x(), pos.x()));
        min.setY(qMin(min.y(), pos.y()));
        min.setZ(qMin(min.z(), pos.z()));

        max.setX(qMax(max.x(), pos.x()));
        max.setY(qMax(max.y(), pos.y()));
        max.setZ(qMax(max.z(), pos.z()));
    }
}

QVector3D Mesh::getBoundingBoxCenter() const
{
    QVector3D min, max;
    getBoundingBox(min, max);
    return (min + max) * 0.5f;
}

QVector3D Mesh::getBoundingBoxSize() const
{
    QVector3D min, max;
    getBoundingBox(min, max);
    return max - min;
}

void Mesh::rotateVertices(const QMatrix4x4 &rotationMatrix)
{
    // // 对每个顶点应用旋转矩阵
    for (auto &vertex : vertices) {
        QVector4D pos(vertex, 1.0f);
        pos = pos * rotationMatrix;
        vertex = pos.toVector3DAffine();
    }

}

void Mesh::normalizeModel() {
    QVector3D center = getBoundingBoxCenter();

    // 平移顶点到原点
    for(auto& pos : vertices) {
        pos -= center;
    }
}

void Mesh::computeNormals()
{
    QVector<QVector3D> temp(vertices.size(), QVector3D(0, 0, 0));

    // 遍历所有三角形
    for (int i = 0; i < indices.size(); i += 3) {
        GLuint i0 = indices[i];
        GLuint i1 = indices[i + 1];
        GLuint i2 = indices[i + 2];

        QVector3D v0 = vertices[i0];
        QVector3D v1 = vertices[i1];
        QVector3D v2 = vertices[i2];

        // 计算面的法线
        QVector3D edge1 = v1 - v0;
        QVector3D edge2 = v2 - v0;
        QVector3D faceNormal = QVector3D::crossProduct(edge1, edge2).normalized();

        // 累加到每个顶点
        temp[i0] += faceNormal;
        temp[i1] += faceNormal;
        temp[i2] += faceNormal;
    }

    // 标准化所有法线
    for (auto& normal : temp) {
        normal.normalize();
    }
    normals = temp;
}

bool Mesh::loadOBJ(const QString& filePath, Mesh& mesh) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Failed to open OBJ file");
        return false;
    }

    // 临时存储原始数据
    QVector<QVector3D> tempVertices;
    QVector<QVector2D> tempTexCoords;
    QVector<QVector3D> tempNormals; // 可选

    // 顶点哈希表（用于去重）
    QHash<QString, GLuint> vertexMap;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        QStringList tokens = line.split(" ", Qt::SkipEmptyParts);
        if (tokens.isEmpty()) continue;

        const QString& type = tokens[0];

        // 解析顶点位置 (v x y z)
        if (type == "v") {
            if (tokens.size() < 4) continue;
            tempVertices.append(QVector3D(
                tokens[1].toFloat(),
                tokens[2].toFloat(),
                tokens[3].toFloat()
                ));
        }
        // 解析纹理坐标 (vt u v)
        else if (type == "vt") {
            if (tokens.size() < 3) continue;
            tempTexCoords.append(QVector2D(
                tokens[1].toFloat(),
                tokens[2].toFloat()
                ));
        }
        // 解析法线 (vn x y z) - 可选
        else if (type == "vn") {
            if (tokens.size() < 4) continue;
            tempNormals.append(QVector3D(
                tokens[1].toFloat(),
                tokens[2].toFloat(),
                tokens[3].toFloat()
                ));
        }
        // 解析面数据 (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)
        else if (type == "f") {
            // 处理每个面的顶点（支持三角形和四边形）
            QVector<GLuint> faceIndices;
            for (int i = 1; i < tokens.size(); ++i) {
                QStringList parts = tokens[i].split("/");

                // 解析索引（OBJ索引从1开始）
                int vIdx = parts[0].toInt() - 1;
                int vtIdx = (parts.size() > 1 && !parts[1].isEmpty()) ?
                                parts[1].toInt() - 1 : -1;
                int vnIdx = (parts.size() > 2) ? parts[2].toInt() - 1 : -1;

                // 创建顶点唯一标识键
                QString key = QString("%1/%2").arg(vIdx).arg(vtIdx);

                // 如果顶点不存在，则添加到输出数据
                if (!vertexMap.contains(key)) {
                    // 确保索引有效
                    if (vIdx >= 0 && vIdx < tempVertices.size()) {
                        mesh.vertices.append(tempVertices[vIdx]);
                    } else {
                        qWarning("Invalid vertex index in face data");
                        continue;
                    }

                    // 添加纹理坐标（无效则填充默认值）
                    if (vtIdx >= 0 && vtIdx < tempTexCoords.size()) {
                        mesh.texCoords.append(tempTexCoords[vtIdx]);
                    } else {
                        mesh.texCoords.append(QVector2D(0.0f, 0.0f));
                    }

                    // 记录新顶点的索引
                    vertexMap[key] = mesh.vertices.size() - 1;
                }

                // 添加到当前面的索引
                faceIndices.append(vertexMap[key]);
            }

            // 三角化多边形面（将四边形拆分为两个三角形）
            for (int i = 2; i < faceIndices.size(); ++i) {
                mesh.indices.append(faceIndices[0]);
                mesh.indices.append(faceIndices[i-1]);
                mesh.indices.append(faceIndices[i]);
            }
        }
    }

    file.close();

    mesh.normalizeModel();
    mesh.computeNormals();

    return true;
}

QOpenGLVertexArrayObject &Mesh::VAO()
{
    return _vao;
}

QOpenGLBuffer &Mesh::VBO()
{
    return _vbo;
}

QOpenGLBuffer &Mesh::TexVbo()
{
    return _texVbo;
}

QOpenGLBuffer &Mesh::IBO()
{
    return _ibo;
}

void Mesh::setupMeshToGraphics(QOpenGLShaderProgram* shader)
{
    if(!_vao.isCreated()) _vao.create();
    if (!_vbo.isCreated()) _vbo.create();
    if (!_ibo.isCreated()) _ibo.create();
    if (!_texVbo.isCreated()) _texVbo.create();
    if (!_normalvbo.isCreated()) _normalvbo.create();
    if(!shader)return;

    bool hasTexture = texture && !texCoords.empty();

    _vao.bind();
    _vbo.bind();
    _vbo.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));
    _vbo.release();

    if(!texCoords.empty()) {
        _texVbo.bind();
        _texVbo.allocate(texCoords.constData(), texCoords.size() * sizeof(QVector2D));
        _texVbo.release();
    }

    // 纹理坐标（如果有）
    if(hasTexture) {
        _texVbo.bind();
        _texVbo.allocate(texCoords.constData(), texCoords.size() * sizeof(QVector2D));
        shader->enableAttributeArray(1);
        shader->setAttributeBuffer(1, GL_FLOAT, 0, 2, sizeof(QVector2D));
        _texVbo.release();
    }else{
        shader->disableAttributeArray(1);
    }

    if(!indices.empty()) {
        _ibo.bind();
        _ibo.allocate(indices.constData(), indices.size() * sizeof(GLuint));
        // _ibo.release();
    }

    if(!normals.empty()){
        _normalvbo.bind();
        _normalvbo.allocate(normals.constData(), normals.size() * sizeof(QVector3D));
        shader->enableAttributeArray(2);
        shader->setAttributeBuffer(2, GL_FLOAT, 0, 3, sizeof(QVector3D));
        _normalvbo.release();
    }

    _vao.release();
}

OpenGLRenderer::OpenGLRenderer()
    : m_shaderProgram(nullptr)
    , m_rotationAngle(0.0f)
{
    m_mesh = nullptr;
    model_active = false;

    // 设置变换矩阵
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, float(m_viewportSize.width()) / float(m_viewportSize.height()), 0.1f, 100.0f);

    m_view.setToIdentity();
    m_view.translate(0.0f, 0.0f, -8.0f);

    initializeOpenGLFunctions();
}

OpenGLRenderer::~OpenGLRenderer()
{
    delete m_shaderProgram;
    if(m_mesh)
        m_mesh->destroy();
}

void OpenGLRenderer::render()
{
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    renderScene();

    m_rotationAngle += 1.0f;
    if(m_rotationAngle>360.f)
        m_rotationAngle = m_rotationAngle-360.f;
    update();
}

QOpenGLFramebufferObject *OpenGLRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}

void OpenGLRenderer::synchronize(QQuickFramebufferObject *item)
{
    Q_UNUSED(item)
    if (!m_shaderProgram) {
        initializeGL();
    }

    QMutexLocker locker(&m_mutex);
    if (!m_pendingFilePath.isEmpty()) {
        // 将数据同步到渲染器
        m_filepath = m_pendingFilePath;
        initalModel();
        model_active = true;
        m_pendingFilePath.clear();

        qDebug() << "同步数据到渲染线程，文件路径:" << m_filepath;
    }
}

void OpenGLRenderer::setFilePath(QString filepath)
{
    QMutexLocker locker(&m_mutex);
    m_pendingFilePath = filepath;
}

void OpenGLRenderer::initalModel()
{
    if(!m_mesh)
    {
        m_mesh = new Mesh();
    }
    if(Mesh::loadOBJ(m_filepath,*m_mesh))
    {
        m_mesh->setupMeshToGraphics(m_shaderProgram);
        float distance = CameraUtils::getAutoAdjustCamera(m_mesh->vertices,45.0f,m_viewportSize.width(),m_viewportSize.height());

        QVector3D viewPos(0.0f, 0.0f, -distance);// 观察者位置
        m_view.setToIdentity();
        m_view.translate(0.0f, 0.0f, -distance);
        m_shaderProgram->setUniformValue("viewPos", viewPos);
    }
}

void OpenGLRenderer::initializeGL()
{
    setupShaderProgram();
    // setupGeometry();
}

void OpenGLRenderer::setupShaderProgram()
{
    m_shaderProgram = new QOpenGLShaderProgram();

    // 顶点着色器（支持纹理坐标）
    // 顶点着色器（添加法线和光照）
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                             "#version 330 core\n"
                                             "layout (location = 0) in vec3 position;\n"
                                             "layout (location = 1) in vec2 texCoord;\n"
                                             "layout (location = 2) in vec3 normal;  \n"
                                             "out vec2 TexCoord;\n"
                                             "out vec3 Normal;\n"
                                             "out vec3 FragPos;\n"
                                             "uniform mat4 projection;\n"
                                             "uniform mat4 view;\n"
                                             "uniform mat4 model;\n"
                                             "void main()\n"
                                             "{\n"
                                             "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
                                             "    FragPos = vec3(model * vec4(position, 1.0));\n"
                                             "    Normal = mat3(transpose(inverse(model))) * normal;\n"
                                             "    TexCoord = texCoord;\n"
                                             "}\n");

    // 片段着色器（支持纹理采样）
    // 片段着色器（添加简单光照）
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                             "#version 330 core\n"
                                             "in vec2 TexCoord;\n"
                                             "in vec3 Normal;\n"
                                             "in vec3 FragPos;\n"
                                             "out vec4 FragColor;\n"
                                             "uniform sampler2D textureSampler;\n"
                                             "uniform bool useTexture;\n"
                                             "uniform vec3 objectColor;\n"
                                             "uniform vec3 lightPos;\n"
                                             "uniform vec3 lightColor;\n"
                                             "uniform vec3 viewPos;\n"
                                             "void main()\n"
                                             "{\n"
                                             "    // 环境光\n"
                                             "    float ambientStrength = 0.2;\n"
                                             "    vec3 ambient = ambientStrength * lightColor;\n"
                                             "    \n"
                                             "    // 漫反射\n"
                                             "    vec3 norm = normalize(Normal);\n"
                                             "    vec3 lightDir = normalize(lightPos - FragPos);\n"
                                             "    float diff = max(dot(norm, lightDir), 0.0);\n"
                                             "    vec3 diffuse = diff * lightColor;\n"
                                             "    \n"
                                             "    // 镜面反射\n"
                                             "    float specularStrength = 0.5;\n"
                                             "    vec3 viewDir = normalize(viewPos - FragPos);\n"
                                             "    vec3 reflectDir = reflect(-lightDir, norm);\n"
                                             "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
                                             "    vec3 specular = specularStrength * spec * lightColor;\n"
                                             "    \n"
                                             "    // 组合光照\n"
                                             "    vec3 result = (ambient + diffuse + specular);\n"
                                             "    \n"
                                             "    if (useTexture) {\n"
                                             "        FragColor = texture(textureSampler, TexCoord) * vec4(result, 1.0);\n"
                                             "    } else {\n"
                                             "        FragColor = vec4(objectColor * result, 1.0);\n"
                                             "    }\n"
                                             "}\n");

    m_shaderProgram->link();
}

void OpenGLRenderer::renderScene()
{
    if (!m_shaderProgram) return;

    m_shaderProgram->bind();

    m_model.setToIdentity();
    m_model.rotate(m_rotationAngle, QVector3D(0.5f, 1.0f, 0.0f));

    m_shaderProgram->setUniformValue("projection", m_projection);
    m_shaderProgram->setUniformValue("view",  m_view);

    // 设置光照参数
    QVector3D lightPos(30.0f, 30.0f, 30.0f);      // 光源位置
    QVector3D lightColor(1.0f, 1.0f, 1.0f);    // 白光

    m_shaderProgram->setUniformValue("lightPos", lightPos);
    m_shaderProgram->setUniformValue("lightColor", lightColor);

    renderMesh();

    m_shaderProgram->release();
}

void OpenGLRenderer::renderMesh()
{
    if(!model_active||!m_mesh)
        return;

    m_shaderProgram->setUniformValue("model", m_model);

    bool hasTexture = m_mesh->texture && !m_mesh->texCoords.empty();

    m_shaderProgram->setUniformValue("useTexture", hasTexture);
    if (hasTexture) {
        m_mesh->texture->bind(0);
        m_shaderProgram->setUniformValue("textureSampler", 0);
    }
    else
    {
        m_shaderProgram->setUniformValue("objectColor", QVector3D(0.7f, 0.7f, 0.7f)); // 灰色
    }

    // 设置顶点数据
    m_mesh->VAO().bind();
    // 使用索引绘制（如果有）
    if(!m_mesh->indices.empty()) {
        glDrawElements(GL_TRIANGLES, m_mesh->indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        // 没有索引则直接绘制顶点
        glDrawArrays(GL_TRIANGLES, 0, m_mesh->vertices.size());
    }
    m_mesh->VAO().release();
}

// OpenGLRenderItem 实现
OpenGLModelRenderItem::OpenGLModelRenderItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    // 重要：设置这个标志以启用渲染
    // setFlag(ItemHasContents, true);
    // setAcceptedMouseButtons(Qt::AllButtons); // 接受鼠标事件

    render = nullptr;

    qDebug() << "OpenGLModelRenderItem created";
}

QQuickFramebufferObject::Renderer *OpenGLModelRenderItem::createRenderer() const
{
    qDebug() << "Creating OpenGLRenderer";
    render = new OpenGLRenderer();
    return render;
}

void OpenGLModelRenderItem::setFilePath(QString filepath)
{
    m_filepath = filepath;
    if(render){
        render->setFilePath(m_filepath);
    }
    // 强制更新
    update();
}
