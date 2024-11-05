#include "Application.h"

#include "Polastri/Renderer/RenderCommand.h"
#include "Polastri/Renderer/Renderer.h"


namespace PT {
    Application* Application::s_Instance = nullptr;
    const char* Application::s_GLVersion = "#version 410";

    Application::Application()
        : m_Camera(-1.5f, 1.5f, -1.0f, 1.0f)
    {
        if (s_Instance)
            PT_ERROR("Application already initialized");
        s_Instance = this;
        m_Window = Ref<Window>(Window::Create());
        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

        m_VertexArray.reset(VertexArray::Create());

        float vertices[3*7] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
             0.0f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
        };

        std::shared_ptr<VertexBuffer> vertexBuffer;
        vertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));
        vertexBuffer->SetLayout({
            {"a_Position", ShaderDataType::Float3},
            {"a_Color", ShaderDataType::Float4},
        });
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        unsigned int indices[3] = { 0, 1, 2 };
        std::shared_ptr<IndexBuffer> indexBuffer;
        indexBuffer.reset(IndexBuffer::Create(indices, 3));
        m_VertexArray->SetIndexBuffer(indexBuffer);

        m_SquareVA.reset(VertexArray::Create());

        float squareVertices[3*4] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f,
        };

        std::shared_ptr<VertexBuffer> squareVB;
        squareVB.reset(VertexBuffer::Create(squareVertices, sizeof(squareVertices)));
        squareVB->SetLayout({
            {"a_Position", ShaderDataType::Float3}
        });
        m_SquareVA->AddVertexBuffer(squareVB);

        unsigned int squareIndices[6] = { 0, 1, 2, 0, 2, 3};
        std::shared_ptr<IndexBuffer> squareIB;
        squareIB.reset(IndexBuffer::Create(squareIndices, 6));
        m_SquareVA->SetIndexBuffer(squareIB);

        std::string vertexSrc = R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;
            out vec3 v_Position;
            out vec4 v_Color;
            uniform mat4 u_ViewProjection;

            void main()
            {
                v_Position = a_Position;
                v_Color = a_Color;
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            }
        )";

        std::string fragmentSrc = R"(
            #version 330 core

            layout(location = 0) out vec4 color;
            in vec3 v_Position;
            in vec4 v_Color;

            void main()
            {
                color = v_Color;
            }
        )";

        m_Shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);

        std::string blueVertexSrc = R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            uniform mat4 u_ViewProjection;

            void main()
            {
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            }
        )";

        std::string blueFragmentSrc = R"(
            #version 330 core

            layout(location = 0) out vec4 color;

            void main()
            {
                color = vec4(0.2, 0.82, 0.2, 1.0);
            }
        )";

        m_BlueShader = std::make_unique<Shader>(blueVertexSrc, blueFragmentSrc);
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_Running)
        {
            RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
            RenderCommand::Clear();

            m_BlueShader->Bind();
            m_BlueShader->UploadUniformMat4("u_ViewProjection", m_Camera.GetViewProjectionMatrix());
            Renderer::Submit(m_SquareVA);

            m_Shader->Bind();
            m_Shader->UploadUniformMat4("u_ViewProjection", m_Camera.GetViewProjectionMatrix());
            Renderer::Submit(m_VertexArray);

            {
                for (Layer* layer : m_LayerStack)
                    layer->OnUpdate();
            }

            m_ImGuiLayer->Begin();
            {
                for (Layer* layer : m_LayerStack)
                    layer->OnImGuiRender();
            }
            m_ImGuiLayer->End();

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer *layer)
    {
        m_LayerStack.PushOverlay(layer);
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;
        // Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return false;
    }
}
