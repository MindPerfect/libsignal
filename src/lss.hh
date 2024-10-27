/*

Original code https://github.com/cpp11nullptr Ievgen Polyvanyi
Cloned to https://github.com/balmerdx/lsignal
Modified 
*/

#pragma once

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>


namespace lss
{
	// connection

	struct connection_data
	{
		bool locked = false;
		bool deleted = false;

		//SSC:
		//connection_data();
		//~connection_data();
	};

	struct connection_cleaner
	{
		std::shared_ptr<connection_data> data;

		//SSC:
		//connection_cleaner();
		//~connection_cleaner();
	};

	class connection
	{
		template<typename>
		friend class signal;

	public:
		connection() {}
		connection(std::shared_ptr<connection_data>&& data)
			: _data(std::move(data)) {}
		virtual ~connection() {} ; // TODO: Really needed?

		bool is_locked() const { return _data->locked; }
		void set_lock(const bool lock) { _data->locked = lock; }

		void disconnect() {
			if (_data) {
				//connection fully cleared after next signal call or signal delete
				_data->deleted = true;
				_data.reset();
			}
		}
		
	private:
		std::shared_ptr<connection_data> _data;
	};


	// slot
	class slot
	{
		template<typename>
		friend class signal;
	public:
		slot() {}
		virtual ~slot() { disconnect(); }

		void disconnect() {
			decltype(_cleaners) cleaners = _cleaners;
			for (auto iter = cleaners.cbegin(); iter != cleaners.cend(); ++iter) {
				const connection_cleaner& cleaner = *iter;
				cleaner.data->deleted = true;
			}

			_cleaners.clear();
		}
	private:
		std::vector<connection_cleaner> _cleaners;
	};

	// signal
	template<typename>
	class signal;

	template<typename R, typename... Args>
	class signal<R(Args...)>
	{
	public:
		using result_type = R;
		using callback_type = std::function<R(Args...)>;

		signal();
		~signal();

		signal(const signal& rhs);
		signal& operator= (const signal& rhs);

		signal(signal&& rhs) = default;
		signal& operator= (signal&& rhs) = default;

		bool is_locked() const;
		void set_lock(const bool lock);

		connection connect(const callback_type& fn, slot *owner);
		connection connect(callback_type&& fn, slot *owner);

		template<typename T, typename Tfn>
		connection connect(T *p, R(Tfn::*fn)(Args...), slot *owner);

		template<typename T, typename Tfn>
		connection connect(T *p, R(Tfn::*fn)(Args...) const, slot *owner);

		void disconnect(const connection& connection);

		void disconnect_all();

		//Return last called signal result.
		R operator() (Args... args) const;

		//this signal don`t have direct connections
		bool empty() const;
	private:
		struct joint
		{
			callback_type callback;
			std::shared_ptr<connection_data> connection;
		};

		struct internal_data
		{
			mutable std::mutex _mutex;
			bool _locked = false;
			int _signal_called_count = 0;

			std::list<joint> _callbacks;
		};

		std::shared_ptr<internal_data> _data;

		void copy_callbacks(const std::list<joint>& callbacks);

		std::shared_ptr<connection_data> create_connection(callback_type&& fn, slot *owner);

		void delete_deffered_internal(internal_data* data) const;

		void add_cleaner(slot *owner, std::shared_ptr<connection_data>& connection) const;
	};

	template<typename R, typename... Args>
	signal<R(Args...)>::signal()
		: _data(std::make_shared<internal_data>())
	{
	}

	template<typename R, typename... Args>
	signal<R(Args...)>::~signal()
	{
	}

	template<typename R, typename... Args>
	void signal<R(Args...)>::disconnect_all()
	{
		internal_data* data = _data.get();
		std::lock_guard<std::mutex> locker(data->_mutex);

		for (auto& jnt : data->_callbacks)
		{
			jnt.connection->deleted = true;
		}

		//data->_callbacks.clear(); dont clear callbacks, only mark deleted
	}

	template<typename R, typename... Args>
	signal<R(Args...)>::signal(const signal& rhs)
		: _data(std::make_shared<internal_data>())
	{
		internal_data* data = _data.get();
		internal_data* rhs_data = rhs._data.get();

		std::unique_lock<std::mutex> lock_own(data->_mutex, std::defer_lock);
		std::unique_lock<std::mutex> lock_rhs(rhs_data->_mutex, std::defer_lock);

		std::lock(lock_own, lock_rhs);
		delete_deffered_internal(rhs_data);

		data->_locked = rhs_data->_locked;

		copy_callbacks(rhs_data->_callbacks);
	}

	template<typename R, typename... Args>
	signal<R(Args...)>& signal<R(Args...)>::operator= (const signal& rhs)
	{
		internal_data* data = _data.get();
		internal_data* rhs_data = rhs._data.get();

		std::unique_lock<std::mutex> lock_own(data->_mutex, std::defer_lock);
		std::unique_lock<std::mutex> lock_rhs(rhs_data->_mutex, std::defer_lock);

		std::lock(lock_own, lock_rhs);
		delete_deffered_internal(rhs_data);

		data->_locked = rhs_data->_locked;

		copy_callbacks(rhs_data->_callbacks);

		return *this;
	}

	template<typename R, typename... Args>
	bool signal<R(Args...)>::is_locked() const
	{
		return _data.get()->_locked;
	}

	template<typename R, typename... Args>
	void signal<R(Args...)>::set_lock(const bool lock)
	{
		_data.get()->_locked = lock;
	}

	template<typename R, typename... Args>
	connection signal<R(Args...)>::connect(const callback_type& fn, slot *owner)
	{
		return create_connection(static_cast<callback_type>(fn), owner);
	}

	template<typename R, typename... Args>
	connection signal<R(Args...)>::connect(callback_type&& fn, slot *owner)
	{
		return create_connection(std::move(fn), owner);
	}

	template<typename R, typename... Args>
	template<typename T, typename Tfn>
	connection signal<R(Args...)>::connect(T *p, R(Tfn::*fn)(Args...), slot *owner)
	{
		auto mem_fn = [fn, p](Args&&... args){ return (p->*fn)(std::forward<decltype(args)>(args)...); };
		return create_connection(std::move(mem_fn), owner);
	}

	template<typename R, typename... Args>
	template<typename T, typename Tfn>
	connection signal<R(Args...)>::connect(T *p, R(Tfn::*fn)(Args...) const, slot *owner)
	{
		auto mem_fn = [fn, p](Args&&... args) { return (p->*fn)(std::forward<decltype(args)>(args)...); };
		return create_connection(std::move(mem_fn), owner);
	}

	template<typename R, typename... Args>
	void signal<R(Args...)>::disconnect(const connection& conn)
	{
		const_cast<connection*>(&conn)->disconnect();
	}

	template<typename R, typename... Args>
	R signal<R(Args...)>::operator() (Args... args) const
	{
		internal_data* data = _data.get();

		typename std::list<joint>::const_iterator cfirst, clast;

		{
			std::lock_guard<std::mutex> locker(data->_mutex);
			if (data->_signal_called_count == 0)
				delete_deffered_internal(data);

			if (data->_locked || data->_callbacks.empty())
				return R();

			data->_signal_called_count++;

			cfirst = data->_callbacks.cbegin();
			clast = data->_callbacks.cend();
			--clast;
		}

		std::shared_ptr<internal_data> data_store(_data);
		if constexpr (std::is_same<R, void>::value)
		{
			for (auto iter = cfirst; ; ++iter)
			{
				const joint& jnt = *iter;

				if (!jnt.connection->locked && !jnt.connection->deleted && jnt.callback)
					jnt.callback(std::forward<Args>(args)...);

				if (iter == clast)
					break;
			}

			{
				std::lock_guard<std::mutex> locker(data->_mutex);
				data->_signal_called_count--;
			}
			return;
		} else
		{
			R r{};
			for (auto iter = cfirst; ; ++iter)
			{
				const joint& jnt = *iter;

				if (!jnt.connection->locked && !jnt.connection->deleted && jnt.callback)
					r = jnt.callback(std::forward<Args>(args)...);

				if (iter == clast)
					break;
			}

			{
				std::lock_guard<std::mutex> locker(data->_mutex);
				data->_signal_called_count--;
			}
			return r;
		}
	}

	template<typename R, typename... Args>
	void signal<R(Args...)>::copy_callbacks(const std::list<joint>& callbacks)
	{
		internal_data* data = _data.get();
		data->_callbacks.clear();
		for (auto iter = callbacks.begin(); iter != callbacks.end(); ++iter)
		{
			const joint& jn = *iter;

			joint jnt;

			jnt.callback = jn.callback;
			jnt.connection = jn.connection;

			data->_callbacks.push_back(std::move(jnt));
		}
	}

	template<typename R, typename... Args>
	void signal<R(Args...)>::add_cleaner(slot *owner, std::shared_ptr<connection_data>& connection) const
	{
		connection_cleaner cleaner;
		cleaner.data = connection;

		if (owner != nullptr)
			owner->_cleaners.emplace_back(cleaner);
	}

	template<typename R, typename... Args>
	std::shared_ptr<connection_data> signal<R(Args...)>::create_connection(callback_type&& fn, slot *owner)
	{
		std::shared_ptr<connection_data> connection = std::make_shared<connection_data>();

		joint jnt;
		jnt.callback = std::move(fn);
		jnt.connection = connection;

		internal_data* data = _data.get();
		std::lock_guard<std::mutex> locker(data->_mutex);
		add_cleaner(owner, connection);

		data->_callbacks.push_back(std::move(jnt));
		return connection;
	}

	template<typename R, typename... Args>
	void signal<R(Args...)>::delete_deffered_internal(internal_data* data) const
	{
		auto it_to_remove = std::remove_if(data->_callbacks.begin(), data->_callbacks.end(),
			[](const joint& jnt) { return jnt.connection->deleted; }
			);

		data->_callbacks.erase(it_to_remove, data->_callbacks.end());
	}

	template<typename R, typename... Args>
	bool signal<R(Args...)>::empty() const
	{
		internal_data* data = _data.get();
		std::lock_guard<std::mutex> locker(data->_mutex);
		return data->_callbacks.empty();
	}
}


namespace lss
{
/*
	connection_data::connection_data()
	{

	}

	connection_data::~connection_data()
	{

	}

	connection_cleaner::connection_cleaner()
	{

	}

	connection_cleaner::~connection_cleaner()
	{

	}

	connection::connection()
	{

	}
	
	connection::connection(std::shared_ptr<connection_data>&& data)
		: _data(std::move(data))
	{
	}

	connection::~connection()
	{
	}

	bool connection::is_locked() const
	{
		return _data->locked;
	}

	void connection::set_lock(const bool lock)
	{
		_data->locked = lock;
	}

	void connection::disconnect()
	{
		if (_data)
		{
			//connection fully cleared after next signal call or signal delete
			_data->deleted = true;
			_data.reset();
		}
	}

	*/

	/*
	slot::slot()
	{
	}

	slot::~slot()
	{
		disconnect();
	}

	void slot::disconnect()
	{
		decltype(_cleaners) cleaners = _cleaners;

		for (auto iter = cleaners.cbegin(); iter != cleaners.cend(); ++iter)
		{
			const connection_cleaner& cleaner = *iter;
			cleaner.data->deleted = true;
		}

		_cleaners.clear();
	}
	*/

} // namespace
