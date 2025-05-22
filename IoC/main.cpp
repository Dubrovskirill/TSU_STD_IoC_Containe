#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <string>
using namespace std;

// IoC-контейнер
class IOCContainer {
    static int s_nextTypeId;
    template<typename T>
    static int GetTypeID() {
        static int typeId = s_nextTypeId++;
        return typeId;
    }

public:
    class FactoryRoot {
    public:
        virtual ~FactoryRoot() {}
    };

    std::map<int, std::shared_ptr<FactoryRoot>> m_factories;

    template<typename T>
    class CFactory : public FactoryRoot {
        std::function<std::shared_ptr<T>()> m_functor;
    public:
        CFactory(std::function<std::shared_ptr<T>()> functor) : m_functor(functor) {}
        std::shared_ptr<T> GetObject() { return m_functor(); }
    };

    template<typename T>
    std::shared_ptr<T> GetObject() {
        auto typeId = GetTypeID<T>();
        auto it = m_factories.find(typeId);
        if (it == m_factories.end()) {
            throw std::runtime_error("Factory for type not registered");
        }
        auto factory = std::static_pointer_cast<CFactory<T>>(it->second);
        return factory->GetObject();
    }

    // Исправленный метод RegisterFactory
    template<typename TInterface>
    void RegisterFactory(std::function<std::shared_ptr<TInterface>()> factoryFunctor) {
        m_factories[GetTypeID<TInterface>()] = std::make_shared<CFactory<TInterface>>(factoryFunctor);
    }
};

int IOCContainer::s_nextTypeId = 0;

enum ProcessorType { x86, x64 };

class IProcessor {
public:
    virtual string GetProcessorInfo() = 0;
    virtual ~IProcessor() = default;
};

class IntelProcessor : public IProcessor {
    string version;
    ProcessorType type;
    double speed;
public:
    IntelProcessor(double speed, ProcessorType type, string version)
        : version(version), type(type), speed(speed) {}
    string GetProcessorInfo() override {
        return "Processor: Intel " + version + ", Type: " + (type == x86 ? "x86" : "x64") + ", Speed: " + to_string(speed) + " GHz";
    }
};

class AMDProcessor : public IProcessor {
    string version;
    ProcessorType type;
    double speed;
public:
    AMDProcessor(double speed, ProcessorType type, string version)
        : version(version), type(type), speed(speed) {}
    string GetProcessorInfo() override {
        return "Processor: AMD " + version + ", Type: " + (type == x86 ? "x86" : "x64") + ", Speed: " + to_string(speed) + " GHz";
    }
};

class Computer {
    shared_ptr<IProcessor> processor;
public:
    Computer(shared_ptr<IProcessor> processor) : processor(processor) {}
    void Configure() {
        if (!processor) {
            throw std::runtime_error("Processor not initialized");
        }
        cout << "Computer configured with " << processor->GetProcessorInfo() << endl;
    }
};

int main() {
    IOCContainer container;

    // Регистрация IntelProcessor
    container.RegisterFactory<IProcessor>([] {
        return std::make_shared<IntelProcessor>(2.5, x64, "i7");
    });

    // Создание и настройка компьютера с IntelProcessor
    auto computer = make_shared<Computer>(container.GetObject<IProcessor>());
    computer->Configure();

    // Регистрация AMDProcessor
    container.RegisterFactory<IProcessor>([] {
        return std::make_shared<AMDProcessor>(3.0, x86, "Ryzen");
    });

    // Создание и настройка компьютера с AMDProcessor
    computer = make_shared<Computer>(container.GetObject<IProcessor>());
    computer->Configure();

    return 0;
}
