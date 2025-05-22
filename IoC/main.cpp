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

    // Регистрация простых фабрик (без зависимостей)
    template<typename TInterface>
    void RegisterSimpleFactory(std::function<std::shared_ptr<TInterface>()> factoryFunctor) {
        m_factories[GetTypeID<TInterface>()] = std::make_shared<CFactory<TInterface>>(factoryFunctor);
    }

    // Регистрация функтора с зависимостями
    template<typename TInterface, typename... TS>
    void RegisterFunctor(std::function<std::shared_ptr<TInterface>(std::shared_ptr<TS>...)> functor) {
        m_factories[GetTypeID<TInterface>()] = std::make_shared<CFactory<TInterface>>(
            [this, functor] { return functor(GetObject<TS>()...); });
    }

    // Регистрация конкретного экземпляра (singleton)
    template<typename TInterface>
    void RegisterInstance(std::shared_ptr<TInterface> instance) {
        m_factories[GetTypeID<TInterface>()] = std::make_shared<CFactory<TInterface>>(
            [instance] { return instance; });
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
    IntelProcessor(std::shared_ptr<double> speedPtr, std::shared_ptr<ProcessorType> typePtr, std::shared_ptr<std::string> versionPtr)
        : version(*versionPtr), type(*typePtr), speed(*speedPtr) {}
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
    AMDProcessor(std::shared_ptr<double> speedPtr, std::shared_ptr<ProcessorType> typePtr, std::shared_ptr<std::string> versionPtr)
        : version(*versionPtr), type(*typePtr), speed(*speedPtr) {}
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

    // Way 1: Simple registration via RegisterSimpleFactory
    cout << "===== Way 1: Simple registration =====" << endl;
    container.RegisterSimpleFactory<IProcessor>([] {
        return make_shared<IntelProcessor>(2.5, x64, "i7");
    });
    auto computer1 = make_shared<Computer>(container.GetObject<IProcessor>());
    computer1->Configure();

    // Way 2: Instance registration via RegisterInstance
    cout << "===== Way 2: Instance registration =====" << endl;
    auto sharedProcessor = make_shared<AMDProcessor>(3.0, x86, "Ryzen");
    container.RegisterInstance<IProcessor>(sharedProcessor);
    auto computer2 = make_shared<Computer>(container.GetObject<IProcessor>());
    computer2->Configure();
    auto computer3 = make_shared<Computer>(container.GetObject<IProcessor>());
    computer3->Configure();

    // Way 3: Registration with dependencies via RegisterFunctor
    cout << "===== Way 3: Registration with dependencies =====" << endl;
    container.RegisterInstance<double>(make_shared<double>(4.0));
    container.RegisterInstance<ProcessorType>(make_shared<ProcessorType>(x64));
    container.RegisterInstance<string>(make_shared<string>("i9"));
    container.RegisterFunctor<IProcessor, double, ProcessorType, string>(
        std::function<std::shared_ptr<IProcessor>(std::shared_ptr<double>, std::shared_ptr<ProcessorType>, std::shared_ptr<std::string>)>(
            [](std::shared_ptr<double> speed, std::shared_ptr<ProcessorType> type, std::shared_ptr<std::string> version) -> std::shared_ptr<IProcessor> {
                return std::make_shared<IntelProcessor>(*speed, *type, *version);
            }
            )
        );
    auto computer4 = make_shared<Computer>(container.GetObject<IProcessor>());
    computer4->Configure();

    return 0;
}
