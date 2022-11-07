Pet_project: HTTP сервис для выдачи файлов по запросу . Список, находящихся в доступе файлов, автоматически поддерживаемый в актуальном состоянии, хранится в БД. Осуществляется логирование http запросов, логирование запросов к БД и логирование изменения состояния файлохранилища.
Client_HTTP - HTTP клиент на Boost.Asio и Boost.Beast c применением stackful coroutines.
DB_module - модуль для работы с БД, содержит пул потоков, пул коннектов, класс, осуществляющий взаимодействие с БД (используется PostgreSql), класс, хранящий результаты запроса и методы раюоты с этим результатом.
Download - директория для загруженных через Client_HTTP файлов.
Handler - содержит классы обработчиков запросов, получаемых http сервером (Server_HTTP), и обработчиков событий, фиксируемых модулем отслеживания состояния папки с предоставляемыми файлами (Inotify_module).
Logger - содержит классы для осуществления логирования событий.
Server_HTTP - HTTP клиент на Boost.Asio и Boost.Beast c применением stackless coroutines, приём и обработка запросов на выдачу списка доступных файлов, таблицы логов, самих файлов.
ini_and_parser - содержит файл для конфигурации всех комопонентов сервиса и классы для парсинга конфигурационного файла.
logs_folder - директория для хранения файлов с логами.
scripts_for_DB - bash скрипты для создания БД и таблиц для сервиса.
songs_folder - директория для хранения доступных файлов.
