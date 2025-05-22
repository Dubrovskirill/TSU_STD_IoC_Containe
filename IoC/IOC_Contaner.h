#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <string>
using namespace std;

class IOCContainer
{
	static int s_nextTypeId;
	template<typename T>
	static int GetTypeID() {
		static int typeId = s_nextTypeId++;
		return typeId;
	}

public:
	//Создание typeid для типа
	/*
	 * В предлагаемой реализации контейнера IOC  есть статическая целочисленная переменная,
	 * указывающая идентификатор следующего типа, который будет выделен,
	 * и экземпляр статической локальной переменной для каждого типа,
	 * доступ к которому можно получить, вызвав метод GetTypeID.
	*/

	/*
	 * Получение экземпляров объекта
	 * Теперь, когда у нас есть идентификатор типа,
	 * мы должны иметь возможность хранить какой-то фабричный объект,
	 * чтобы представить тот факт, что мы не знаем, как создать этот объект.
	 * Поскольку я хочу хранить все фабрики в одной коллекции,
	 * я выбираю абстрактный базовый класс, от которого будут производными фабрики,
	 * и реализацию, которая фиксирует функтор для последующего вызова.
	 * Для краткости я использовал std::map для хранения фабрик, однако
	 * вы можете рассмотреть и другие варианты для повышения эффективности.
	 */
	
	class FactoryRoot
	{
	public:
		virtual ~FactoryRoot() {}
	};


	std::map<int, std::shared_ptr<FactoryRoot>> m_factories;

	//Получить экземпляр объекта
	template<typename T>
	class CFactory : public FactoryRoot
	{
		std::function<std::shared_ptr<T>()> m_functor;

	public:
		~CFactory() {}

		CFactory(std::function<std::shared_ptr<T>()> functor)
			: m_functor(functor)
		{}

		std::shared_ptr<T> GetObject() {
			return m_functor();
		}
	};

	template<typename T>
	std::shared_ptr<T> GetObject() {
		auto typeId = GetTypeID<T>();
		auto factoryBase = m_factories[typeId];
		auto factory = std::static_pointer_cast<CFactory<T>>(factoryBase);
		return factory->GetObject();
	}

	//Регистрация экземпляров

	//Самая простая реализация - зарегистрировать функтор
	template<typename TInterface, typename... TS>
	void RegisterFunctor(
		std::function<std::shared_ptr<TInterface>(std::shared_ptr<TS>... ts)> functor) {
		m_factories[GetTypeID<TInterface>()] = std::make_shared<CFactory<TInterface>>(
				[ = ] { return functor(GetObject<TS>()...); });
	}

	//Регистрация одного экземпляра объекта
	template<typename TInterface>
	void RegisterInstance(std::shared_ptr<TInterface> t) {
		m_factories[GetTypeID<TInterface>()] = std::make_shared<CFactory<TInterface>>(
				[ = ] { return t; });
	}

	//Подаем указатель на функцию
	template<typename TInterface, typename... TS>
	void RegisterFunctor(std::shared_ptr<TInterface> (*functor)(std::shared_ptr<TS>... ts)) {
		RegisterFunctor(
			std::function<std::shared_ptr<TInterface>(std::shared_ptr<TS>... ts)>(functor));
	}

	//Фабрика, которая будет вызывать конструктор, для каждого экземпляра
	template<typename TInterface, typename TConcrete, typename... TArguments>
	void RegisterFactory() {
		RegisterFunctor(
			std::function<std::shared_ptr<TInterface>(std::shared_ptr<TArguments>... ts)>(
		[](std::shared_ptr<TArguments>... arguments) -> std::shared_ptr<TInterface> {
			return std::make_shared<TConcrete>(
				std::forward<std::shared_ptr<TArguments>>(arguments)...);
		}));
	}

	//Фабрика, которая будет возвращать один экземпляр
	template<typename TInterface, typename TConcrete, typename... TArguments>
	void RegisterInstance() {
		RegisterInstance<TInterface>(std::make_shared<TConcrete>(GetObject<TArguments>()...));
	}
};

IOCContainer gContainer;

// инициализируем ненулевым числом
int IOCContainer::s_nextTypeId = 115094801;

class IAmAThing
{
public:
	virtual ~IAmAThing() {}
	virtual void TestThis() = 0;
};

class IAmTheOtherThing
{
public:
	virtual ~IAmTheOtherThing() {}
	virtual void TheOtherTest() = 0;
};

class TheThing : public IAmAThing
{
public:
	TheThing() {}
	void TestThis() {
		std::cout << "A Thing" << std::endl;
	}
};

class TheOtherThing : public IAmTheOtherThing
{
	std::shared_ptr<IAmAThing> m_thing;
	string m_superInfo;

public:
	TheOtherThing(std::shared_ptr<IAmAThing> thing)
		: m_thing(thing) {
		m_superInfo = "From TheOtherThing";
	}
	TheOtherThing(std::shared_ptr<IAmAThing> thing, string sInfo)
		: m_thing(thing)
		, m_superInfo(sInfo) {
		m_superInfo = sInfo;
	}
	void TheOtherTest() {
		std::cout << m_superInfo << std::endl;
		m_thing->TestThis();
	}
};

class IHello
{
public:
	virtual void hello() const = 0;
	virtual ~IHello() = default;
};

class Hello : public IHello
{
public:
	virtual void hello() const {
		std::cout << "hello world!" << std::endl;
	}
};

class Privet : public IHello
{
public:
	virtual void hello() const {
		std::cout << "Privet world!" << std::endl;
	}
};


int main(int argc, const char *argv[])
{
	//------Example #1----------------
	//Injector injector;
	IOCContainer injector;

	// Регистрируем IHello с классом Hello, т.о. каждый раз запрашивая IHell получаем объект Hello.
	injector.RegisterInstance<IHello, Hello>();
	auto helloInstance = injector.GetObject<IHello>();
	helloInstance->hello();
	injector.RegisterInstance<IHello, Privet>();

	//Здесь, после регистрации получим объект Privet
	helloInstance = injector.GetObject<IHello>();
	helloInstance->hello();

	//------Example #2----------------

	gContainer.RegisterInstance<IAmAThing, TheThing>();
	gContainer.RegisterFactory<IAmTheOtherThing, TheOtherThing, IAmAThing>();

	gContainer.GetObject<IAmAThing>()->TestThis();
	gContainer.GetObject<IAmTheOtherThing>()->TheOtherTest();

	//Опять запршиваем объект,после последней регистрации получим объект Privet
	helloInstance = injector.GetObject<IHello>();
	helloInstance->hello();
	return 0;
}
