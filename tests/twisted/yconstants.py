YTST_ACCOUNT_MANAGER_PATH = "/com/meego/xpmn/ytstenut/AccountManager"
YTST_ACCOUNT_MANAGER_IFACE = "com.meego.xpmn.ytstenut.AccountManager"
YTST_ACCOUNT_PATH = "/org/freedesktop/Telepathy/Account/salut/local_ytstenut/automatic_account"

STATUS_IFACE = 'com.meego.xpmn.ytstenut.Status'

CHANNEL_IFACE = 'com.meego.xpmn.ytstenut.Channel'

REQUEST_TYPE = CHANNEL_IFACE + '.RequestType'
REQUEST_ATTRIBUTES = CHANNEL_IFACE + '.RequestAttributes'
REQUEST_BODY = CHANNEL_IFACE + '.RequestBody'
TARGET_SERVICE = CHANNEL_IFACE + '.TargetService'
INITIATOR_SERVICE = CHANNEL_IFACE + '.InitiatorService'

REQUEST_TYPE_GET = 1
REQUEST_TYPE_SET = 2

ERROR_TYPE_CANCEL = 1
ERROR_TYPE_CONTINUE = 2
ERROR_TYPE_MODIFY = 3
ERROR_TYPE_AUTH = 4
ERROR_TYPE_WAIT = 5

MESSAGE_NS = 'urn:ytstenut:message'
STATUS_NS = 'urn:ytstenut:status'

CAPABILITIES_PREFIX = 'urn:ytstenut:capabilities:'
