###### Pet_project: HTTP сервис для выдачи файлов по запросу . Список, находящихся в доступе файлов, автоматически поддерживаемый в актуальном состоянии, хранится в БД. Осуществляется логирование http запросов, логирование запросов к БД и логирование изменения состояния файлохранилища.  

<sub> Содержит:  
<sub>- **DB_module** - модуль для работы с БД, содержит пул потоков, пул коннектов, класс, осуществляющий взаимодействие с БД (используется PostgreSql), <sub>класс, хранящий результаты запроса и методы раюоты с этим результатом.  
<sub>- **Download** - директория для загруженных через Client_HTTP файлов.  
<sub>- **Handler** - содержит классы обработчиков запросов, получаемых http сервером (Server_HTTP), и обработчиков событий, фиксируемых модулем <sub>отслеживания состояния папки с предоставляемыми файлами (Inotify_module).  
<sub>- **Inotify_module** - модуль для отслеживания изменений в директории с файлами для выдачи, на основе подсистемы Inotify ядра Linux.  
<sub>- **Logger** - содержит классы для осуществления логирования событий.  
<sub>- **Server_HTTP** - HTTP клиент на Boost.Asio и Boost.Beast c применением stackless coroutines, приём и обработка запросов на выдачу списка доступных <sub>файлов, таблицы логов, самих файлов.  
<sub>- **ini_and_parser** - содержит файл для конфигурации всех комопонентов сервиса и классы для парсинга конфигурационного файла.  
<sub>- **logs_folder** - директория для хранения файлов с логами.  
<sub>- **scripts_for_DB** - bash скрипты для создания БД и таблиц для сервиса.  
<sub>- **songs_folder** - директория для хранения доступных файлов.
